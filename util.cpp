#include <numeric>

#include "util.h"

int checksum(char *msg, size_t len)
{
	return int(std::accumulate(msg, msg + len, (unsigned char) 0));
}

int checksum(const Packet& packet)
{
	return int(std::accumulate(packet.data, packet.data + (size_t)packet.size(), (unsigned char) 0));
}

std::string packet_string(const Packet& packet)
{
	char temp[PACKET_SIZE];
	memset(temp, '\0', PACKET_SIZE);
	memcpy(temp, packet.data, packet.size());
	std::string result(temp);
	return result;
}

std::string packet_string(const Packet& packet, size_t size)
{
	size_t amount = packet.size();
	if (amount > size)
		amount = size;
	char temp[PACKET_SIZE];
	memset(temp, '\0', PACKET_SIZE);
	memcpy(temp, packet.data, amount);
	std::string result(temp);
	return result;
}