/// @file server.cpp
///
/// Creates a server process that listens on a certain port for a client to connect
/// and begin transmitting a file.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <errno.h>
#include <numeric>

#include "server.h"
#include "timers.h"
#include "util.h"

#define PACKET_SIZE 512
#define SEQ_NUM 32
#define WINDOW_SIZE 16

#define SERVER_PORT 10050
#define SERVER_TIMEOUT_MSEC

#define ACK 0
#define NAK 1
#define GET 2
#define PUT 3
#define DEL 4
#define TRN 5

/* Prototypes */
int checksum(char *, size_t);

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		std::cerr << "Error: No command line arguments expected" << std::endl;
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
    int sockfd = -1; 

    socklen_t slen = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
    	perror("Error: Could not create socket\n");
    	exit(EXIT_FAILURE);
    }

    // Set the receiving function to non-blocking
    int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1)
    {
    	fprintf(stderr, "Error: Could not bind to port\n");
    	close(sockfd);
    	exit(EXIT_FAILURE);
    }

    printf("Successfully bound server to port %d and listening for clients...\n\n", SERVER_PORT);
	
	bool wait_put = true;
	FILE *outfile;
	bool send_ack = false;
    bool send_nack = false;
    bool reset_server = false;
	int exp_seq = 0;
    int cur_seq = 0;
    
    while(1)
    {	
        if (recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*)&client_addr, &slen) == -1)
		{
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (!wait_put) {
                    std::cout << "TIMEOUT: Cancelling current client transfer" << std::endl << std::endl;
                    std::cout << "Waiting for client connection......" << std::endl << std::endl;
                    wait_put = true;
                }
                continue; /* repeat the while loop */
            } else {
    	    	std::cerr << "Error: Could not receive from client" << std::endl;
                close(sockfd);
                exit(EXIT_FAILURE);                
            }

        }

        send_ack = false;
        send_nack = false;
        reset_server = false;

        //Getting packet sections
        uint8_t * packet_type = (uint8_t*) (buffer + 0);
        uint8_t * seq_num = (uint8_t*) (buffer + 1);
        uint16_t * chk = (uint16_t*) (buffer + 2);
        uint16_t * data_size = (uint16_t*) (buffer + 4);
        char* data = (char*) (buffer + 6);
        cur_seq = (int) *seq_num;

        char terminated_data[PACKET_SIZE-6+1];
        char output[49];
        memset(terminated_data,'\0',PACKET_SIZE-6+1);
        memcpy(terminated_data, data, *data_size);
        memset(output, '\0', 49);
        memcpy(output, terminated_data, 48);
        
        switch(*packet_type) {
            case ACK:
                std::cout << "ACKNOWLEDGE: Packet discarded" << std::endl << std::endl;
            break;
            case NAK:
                std::cout << "NOT ACKNOWLEDGE: Packet discarded" << std::endl << std::endl;
            break;
            case GET:
                std::cout << "GET: Packet discarded" << std::endl << std::endl;
            break;
            case PUT:
                if(wait_put) {
                    std::cout << "PUT: Begin transfer of file \"" << terminated_data << "\"" << std::endl << std::endl;
                    outfile = fopen(terminated_data, "wb");
                    wait_put = false;
                    send_ack = true;
                    exp_seq = 0;
                }
                else {
                    std::cout << "PUT: Connection already in use...request discarded" << std::endl << std::endl;
                    send_ack = false;
                }
            break;
            case DEL:
                std::cout << "DEL: Packet discarded" << std::endl << std::endl;
            break;
            case TRN:
                if(!wait_put) {
                    if(exp_seq == cur_seq) {
                        int recv_chk = checksum(data, *data_size);
                        if (*chk == recv_chk) {
                            if(*data_size > 0) {
                                fwrite(data, 1, *data_size, outfile);
                                std::cout << "RECEIVED: sequence " << (int)cur_seq << std::endl;
                                std::cout << "Data: " << std::endl << output << std::endl << std::endl;
                                //Updating the expected sequence number.
                                exp_seq = (exp_seq + 1) % 2;
                            }
                            else {
                                fclose(outfile);
                                wait_put = true;
                                std::cout << "RECEIVED: close packet for file transfer: closing transfer" << std::endl << std::endl;
                                exp_seq = 0;
                                reset_server = true;
                            }
	                        send_ack = true;
                        }
                        else {
                            std::cout << "RECEIVED: sequence " << (int)cur_seq << ": damaged packet" << std::endl << std::endl;
                            send_nack = true;
                        }
                    }
                    else {
                        std::cout << "RECEIVED: sequence " << (int)cur_seq << ": incorrect sequence number" << std::endl << std::endl;
                        send_ack = false;
                    }
                }
                else {
                    std::cout << "RECEIVED DATA: must first send PUT command" << std::endl << std::endl;
                    send_ack = false;
                }
            break;
            default:
                std::cout << "UNKNOWN PACKET TYPE: Packet discarded" << std::endl << std::endl;
            break;
        }


		if(send_ack) {
			bzero(buffer, 128);

			*packet_type = ACK;
			*seq_num = cur_seq;
            *data_size = 0;

            std::cout << "SENDING ACK: sequence " << cur_seq << std::endl << std::endl << std::endl;
		
			if (sendto(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*) &client_addr, slen) == -1)
			{
				perror("Error: could not send acknowledge to client\n");
				close(sockfd);
				exit(EXIT_FAILURE);
			}
		}
        else if (send_nack) {
            bzero(buffer, 128);

            *packet_type = NAK;
            *seq_num = cur_seq;
            *data_size = 0;

            std::cout << "SENDING NAK: sequence " << cur_seq << std::endl << std::endl << std::endl;
        
            if (sendto(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*) &client_addr, slen) == -1)
            {
                perror("Error: could not send acknowledge to client\n");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }

        if (reset_server) {
            std::cout << "Waiting for client connection......" << std::endl << std::endl;
        }
    }
 
    close(sockfd);
    exit(EXIT_SUCCESS);
}

int checksum(char *msg, size_t len) {
	return int(std::accumulate(msg, msg + len, (unsigned char) 0));
}