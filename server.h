#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "util.h"
#include "timers.h"

using std::vector;

#define FINE 0
#define LOST 1
#define DELAYED 2

std::string packet_string(Packet& packet);
std::string packet_string(Packet& packet, size_t size);
void receive_commands(int sockfd, GremlinInfo& info);
void parse_file(std::string& filename, vector<Packet>& packets, vector<Timer>& timers);
int send_packet(int sockfd, struct sockaddr_in client_addr, Packet& packet, GremlinInfo& info);
void receive_success_msg(int sockfd, struct sockaddr_in client_addr);
bool send_file(std::string filename, int sockfd, struct sockaddr_in client_addr, GremlinInfo& info);

int gremlin(char *data, int corrupt_chance, int loss_chance, int delay_chance);


#endif