/// @file client.cpp
///
/// Creates a client process that listens on a certain port for a server to
/// begin transmitting a file.
///
/// @author Tavis Maclellan

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include <stdint.h>
#include <errno.h>
#include <iostream>
#include <numeric>

#include "client.h"
#include "util.h"
#include "timers.h"

//#define SERVER_PORT 10050

#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 3000
#define CLIENT_SERVER_DEAD_TIMEOUT_MS 5000


std::string packet_string(Packet& packet)
{
    char temp[PACKET_SIZE];
    memset(temp, '\0', PACKET_SIZE);
    memcpy(temp, packet.data(), packet.size());
    std::string result(temp);
    return result;
}

std::string packet_string(Packet& packet, size_t size)
{
    size_t amount = packet.size();
    if (amount > size)
        amount = size;
    char temp[PACKET_SIZE];
    memset(temp, '\0', PACKET_SIZE);
    memcpy(temp, packet.data(), amount);
    std::string result(temp);
    return result;
}

int main(int argc, char** argv)
{
    if(argc != 6) {
        std::cout << "Usage: " << argv[0] << " ";
        std::cout << "<client-port> <server-IP> <server-port> <func> <filename> \n";
        exit(EXIT_FAILURE);
    }

    unsigned short client_port = (unsigned short) strtoul(argv[1], NULL, 0);
    unsigned short server_port = (unsigned short) strtoul(argv[3], NULL, 0);
    char* type = argv[4];
    char* filename = argv[5];

    int sockfd;
    uint8_t packet_type = 255;
    struct sockaddr_in server;
    struct sockaddr_in client;
    socklen_t slen = sizeof(server);

    // Parse the given server IP address
    if(inet_aton(argv[2], &server.sin_addr) == 0)
    {
        std::cerr << "Error: Given IP address not valid: " << argv[2] << std::endl;
        exit(EXIT_FAILURE);
    }

    // Parse the function code
    else if (!strcmp(type, "GET"))
        packet_type = GET;

    if (packet_type == 255)
    {
        std::cerr << "Error: Given function command not valid: " << type << std::endl;
        std::cerr << "Please use GET" << std::endl;
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error: Could not create client socket.\n\n";
        exit(EXIT_FAILURE);
    }

    // Set the timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = TIMEOUT_USEC;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

    // Set all the information on the client address struct
    client.sin_family = AF_INET;
    client.sin_port = htons(client_port);
    client.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*) &client, sizeof(client)) == -1)
    {
        std::cerr << "Error: Could not bind client process to port " << client_port << std::endl;
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);

    std::cout << "Attempting to talk with server at " << argv[2] << ":" << argv[3] << std::endl;
    std::string str_filename(filename);

    request_func(sockfd, filename, str_filename.size(), server);
    //COMMENCE LISTENING
    receive_func(sockfd, filename, server);
}


// request file
void request_func(int sockfd, char* filename, size_t length, sockaddr_in server)
{
    socklen_t slen = sizeof(server);
    Packet packet = Packet(filename, length, 0, GET);

    if (sendto(sockfd, packet.buffer, PACKET_SIZE, 0, (struct sockaddr*) &server, slen) == -1)
    {
        perror("Error: could not send acknowledge to client\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

// receive file
void receive_func(int sockfd, char* filename, sockaddr_in server)
{
    socklen_t slen = sizeof(server);
    Packet packet;
    int cur_seq;
    int exp_seq = 0;
    bool send_ack = false;
    bool send_nak = false;
    FILE *outfile;
    outfile = fopen(filename, "wb");

    bool running = true;
    int timeout_amount = 0;
    while(running)
    {

        if (recvfrom(sockfd, packet.buffer, PACKET_SIZE, 0, (struct sockaddr*)&server, &slen)==-1)
        {
            if (errno == EWOULDBLOCK)
            {
                errno = 0;
                std::cout << "TIMEOUT: Server not responding...\n\n";
                timeout_amount++;
                if (timeout_amount > 15)
                {
                    fprintf(stderr, "Error: Server not responding...ending program\n");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                continue;
            }
            else
            {
                fprintf(stderr, "Error: Could not receive from client\n");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }

        timeout_amount = 0;

        uint8_t packet_type = packet.type();
        uint8_t seq_num = packet.sequence();
        uint16_t checksum = packet.checksum();
        uint16_t data_size = packet.size();
        char* data = packet.data();
        cur_seq = (int) seq_num;

        send_ack = false;
        send_nak = false;

        // Switch behavior based on packet type
        switch(packet_type) {
            case ACK:
                std::cout << "ACKNOWLEDGE: Packet discarded" << std::endl << std::endl;
            break;
            case NAK:
                std::cout << "NOT ACKNOWLEDGE: Packet discarded" << std::endl << std::endl;
            break;
            case GET:
                std::cout << "GET: Packet discarded" << std::endl << std::endl;
            break;
            break;
            break;
            case TRN:
                if(exp_seq == cur_seq) {
                    if(checksum == calc_checksum(packet)) {
                        if(data_size > 0)
                        {
                            std::cout << "RECEIVED: sequence " << (int)cur_seq << "\n";
                            std::cout << "DATA:\n\n";
                            std::cout << packet_string(packet, 48) << "\n\n";
                            fwrite(data, 1, data_size, outfile);
                            //Updating the expected sequence number.
                            exp_seq = (exp_seq + 1) % SEQ_NUM;
                        }
                        else
                        {
                            fclose(outfile);
                            exp_seq = (exp_seq + 1) % SEQ_NUM;
                            std::cout << "RECEIVED: close packet for file transfer: closing transfer" 
                                << std::endl << std::endl;
                            running = false;
                            send_ack = true;
                            break;
                        }
                        send_ack = true;
                    }
                    else {
                        std::cout << "DAMAGED: sequence " << (int)cur_seq << ": damaged packet"
                            << std::endl << std::endl;
                        send_nak = true;
                    }
                }
                else {
                    std::cout << "OUT OF ORDER: sequence " << (int)cur_seq << ": incorrect sequence number" 
                        << std::endl << std::endl;
                    send_ack = true;
                }
            break;
            default:
                printf("UNKNOWN PACKET TYPE: Packet discarded");
            break;
        }

        // Send ACK for received packet
        if(send_ack) {
            packet = Packet(exp_seq, ACK);
            std::cout << "SENDING ACK: sequence " << (int)exp_seq << std::endl << std::endl;
        
            if (sendto(sockfd, packet.buffer, PACKET_SIZE, 0, (struct sockaddr*) &server, slen) == -1)
            {
                perror("Error: could not send acknowledge to server\n");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }
        // Send NAK for damaged packet
        else if (send_nak) {
            packet = Packet(exp_seq, NAK);            
            std::cout << "SENDING NAK: sequence " << (int)exp_seq << std::endl << std::endl;
        
            if (sendto(sockfd, packet.buffer, PACKET_SIZE, 0, (struct sockaddr*) &server, slen) == -1)
            {
                perror("Error: could not send acknowledge to server\n");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }

    }
    // Sending success message for completion
    char* msg = SUCCESS_MSG;
    std::string ttemp(SUCCESS_MSG);
    packet = Packet(msg, ttemp.size(), 0, GET);
    std::cout << "SENDING SUCCESS MSG" << std::endl;
    if (sendto(sockfd, packet.buffer, PACKET_SIZE, 0, (struct sockaddr*) &server, slen) == -1)
    {
        perror("Error: could not send acknowledge to server\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}