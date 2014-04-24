#include <string.h>
#include <numeric>
#include "util.h"

Packet::Packet()
{
}

Packet::Packet(uint8_t sequence, uint8_t type)
{
	bzero(buffer, PACKET_SIZE);

	char* segment = (char*)(buffer + 6);
	uint16_t* size = (uint16_t*)(buffer + 4);
	uint16_t* chksum = (uint16_t*)(buffer + 2);
	uint8_t* seq = (uint8_t*)(buffer + 1);
	uint8_t* packet_type = (uint8_t*)(buffer + 0);

	*size = 0;
	*seq = sequence;
	*packet_type = type;
	*chksum = calc_checksum(segment, (uint16_t)*size);
}

Packet::Packet(char* segment, uint16_t length, uint8_t sequence, uint8_t type)
{
	char* pack = (char*)(buffer + 6);
	uint16_t* size = (uint16_t*)(buffer + 4);
	uint16_t* chksum = (uint16_t*)(buffer + 2);
	uint8_t* seq = (uint8_t*)(buffer + 1);
	uint8_t* packet_type = (uint8_t*)(buffer + 0);

	bzero(buffer, PACKET_SIZE);
	memcpy(pack, segment, length);
	*size = length;
	*seq = sequence;
	*packet_type = type;
	*chksum = calc_checksum(pack, (uint16_t)*size);
}

int calc_checksum(char *msg, size_t len)
{
	return int(std::accumulate(msg, msg + len, (unsigned char) 0));
}

int calc_checksum(Packet& packet)
{
	char* data = packet.data();
	return int(std::accumulate(data, data + (size_t)(packet.size()), (unsigned char) 0));
}

// std::string packet_string(const Packet& packet)
// {
// 	char temp[PACKET_SIZE];
// 	memset(temp, '\0', PACKET_SIZE);
// 	memcpy(temp, packet.data, packet.size());
// 	std::string result(temp);
// 	return result;
// }

// std::string packet_string(const Packet& packet, size_t size)
// {
// 	size_t amount = packet.size();
// 	if (amount > size)
// 		amount = size;
// 	char temp[PACKET_SIZE];
// 	memset(temp, '\0', PACKET_SIZE);
// 	memcpy(temp, packet.data, amount);
// 	std::string result(temp);
// 	return result;
// }