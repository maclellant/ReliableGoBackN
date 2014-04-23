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

struct Packet
{
	char buffer[PACKET_SIZE];

	uint8_t type() { return *((uint8_t*)(data + 0)); }
	uint8_t sequence() { return *((uint8_t*)(data + 1)); }
	uint16_t checksum() { return *((uint16_t*)(data + 2)); }
	uint16_t size() { return *((uint16_t*)(data + 4)); }
	char* data() { return (char*)(buffer + 6); }

	Packet(uint8_t sequence, uint8_t type)
	{
		bzero(buffer, PACKET_SIZE);

		uint16_t* size = (uint16_t*)(buffer + 4);
		uint16_t* checksum = (uint16_t*)(buffer + 2);
		uint8_t* seq = (uint8_t*)(buffer + 1);
		uint8_t* packet_type = (uint8_t*)(buffer + 0);

		*size = 0;
		*seq = sequence;
		*packet_type = type;
		*checksum = 0;
	}

	Packet(char* segment, uint16_t length, uint8_t sequence, uint8_t type)
	{
		char* data = (char*)(buffer + 6);
		uint16_t* size = (uint16_t*)(buffer + 4);
		uint16_t* checksum = (uint16_t*)(buffer + 2);
		uint8_t* seq = (uint8_t*)(buffer + 1);
		uint8_t* packet_type = (uint8_t*)(buffer + 0);

		bzero(buffer, PACKET_SIZE);
		memcpy(data, segment, length);
		*size = length;
		*seq = sequence;
		*packet_type = type;
		*checksum = checksum(data, (uint16_t)*size);
	}
};

std::string packet_string(const Packet& packet);
std::string packet_string(const Packet& packet, size_t size);

int checksum(char *msg, size_t len);
int checksum(const Packet& packet, size_t len);

#endif