#ifndef UTIL_H
#define UTIL_H

#define ACK 0
#define NAK 1
#define GET 2
#define TRN 3

#define PACKET_SIZE 512
#define SEQ_NUM 32
#define WINDOW_SIZE 16

struct Packet
{
	char[PACKET_SIZE] data;
};

int checksum(char *msg, size_t len);
int checksum(const Packet& packet, size_t len);

#endif