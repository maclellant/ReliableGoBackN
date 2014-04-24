#ifndef CLIENT_H
#define CLIENT_H

#include <string.h>
#include "util.h"

std::string packet_string(Packet& packet);
std::string packet_string(Packet& packet, size_t size);
void request_func(int sockfd, char* filename, size_t length, sockaddr_in server);
void receive_func(int sockfd, char* filename, sockaddr_in server);


#endif