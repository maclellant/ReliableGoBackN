#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ACK 0
#define NAK 1
#define GET 2
#define TRN 3

#define PACKET_SIZE 512
#define SEQ_NUM 32
#define WINDOW_SIZE 16

struct Packet
{
	char buffer[PACKET_SIZE];

	uint8_t type() { return *((uint8_t*)(data + 0)); }
	uint8_t sequence() { return *((uint8_t*)(data + 1)); }
	uint16_t checksum() { return *((uint16_t*)(data + 2)); }
	uint16_t size() { return *((uint16_t*)(data + 4)); }
	char* data() { return (char*)(buffer + 6); }
};

std::string packet_string(const Packet& packet);
std::string packet_string(const Packet& packet, size_t size);

int checksum(char *msg, size_t len);
int checksum(const Packet& packet, size_t len);

#endif