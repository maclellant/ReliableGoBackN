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

//#define SERVER_PORT 10050

#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 3000
#define SEQUENCE_COUNT 32
#define PACKET_SIZE 512

#define ACK 0
#define NAK 1
#define GET 2
#define PUT 3
#define DEL 4
#define TRN 5

/* Prototypes */

/*
client
client port
server ip
server port
function (get, put)
file name
corrupt percent
loss percent
delay percent
delay amount
*/
int main(int argc, char** argv)
{
	if(argc != 10) {
		std::cout << "Usage: " << argv[0] << " ";
        std::cout << "<client-port> <server-IP> <server-port> <func> <filename> ";
        std::cout << "<corrupt %%> <loss %%> <delay %%> <delay-amount>" << std::endl;
        exit(EXIT_FAILURE);
	}

	unsigned short client_port = (unsigned short) strtoul(argv[1], NULL, 0);
	unsigned short server_port = (unsigned short) strtoul(argv[3], NULL, 0);
	char* type = argv[4];
    char* filename = argv[5];
    float corrupt_chance = atof(argv[6]);
    float loss_chance = atof(argv[7]);

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
    if (!strcmp(type, "PUT"))
    	packet_type = PUT;
    else if (!strcmp(type, "GET"))
        packet_type = GET;
    else if (!strcmp(type, "DEL"))
        packet_type = DEL;

    if (packet_type == 255)
    {
    	std::cerr << "Error: Given function command not valid: " << type << std::endl;
    	std::cerr << "Please use GET" << std::endl;
    	exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
    	error("Error: Could not create client socket.");
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

    std:cout << "Attempting to talk with server at " << argv[2] << ":" << argv[3] << std:endl;

    //COMMENCE LISTENING
}




