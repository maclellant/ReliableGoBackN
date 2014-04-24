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
#define HEADER_SIZE 6
#define SEQ_NUM 32
#define WINDOW_SIZE 16

#define SUCCESS_MSG "successfully completed"

struct GremlinInfo
{
	int delay_chance;
	int loss_chance;
	int corrupt_chance;
	int delay_amount_ms;
};

struct Packet
{
	char buffer[PACKET_SIZE];

	uint8_t type() { return *((uint8_t*)(buffer + 0)); }
	uint8_t sequence() { return *((uint8_t*)(buffer + 1)); }
	uint16_t checksum() { return *((uint16_t*)(buffer + 2)); }
	uint16_t size() { return *((uint16_t*)(buffer + 4)); }
	char* data() { return (char*)(buffer + 6); }

	Packet();
	Packet(uint8_t sequence, uint8_t type);
	Packet(char* segment, uint16_t length, uint8_t sequence, uint8_t type);
};

int calc_checksum(char *msg, size_t len);
int calc_checksum(Packet& packet);

// std::string packet_string(const Packet& packet);
// std::string packet_string(const Packet& packet, size_t size);

#endif