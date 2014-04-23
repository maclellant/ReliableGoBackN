#include <numeric>

#include "util.h"

int checksum(char *msg, size_t len)
{
	return int(std::accumulate(msg, msg + len, (unsigned char) 0));
}

int checksum(const Packet& packet, size_t len)
{
	return int(std::accumulate(packet.data, packet.data + len, (unsigned char) 0));
}