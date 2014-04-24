/// @file server.cpp
///
/// Creates a server process that listens on a certain port for a client to connect
/// and begin transmitting a file.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <errno.h>
#include <vector>
#include <fcntl.h>

#include "server.h"
#include "timers.h"
#include "util.h"

using std::vector;

#define SERVER_PORT 10050
#define SERVER_TIMEOUT_MSEC 10
#define SERVER_CANCEL_TIMEOUT_COUNT 10
#define SERVER_CLOSE_TIMEOUT_MSEC 2000

std::string packet_string(Packet& packet)
{
    char temp[PACKET_SIZE];
    memset(temp, '\0', PACKET_SIZE);
    memcpy(temp, packet.data(), packet.size());
    std::string result(temp);
    return result;
}

std::string packet_string(Packet& packet, size_t size)
{
    size_t amount = packet.size();
    if (amount > size)
        amount = size;
    char temp[PACKET_SIZE];
    memset(temp, '\0', PACKET_SIZE);
    memcpy(temp, packet.data(), amount);
    std::string result(temp);
    return result;
}

void receive_commands(int sockfd, GremlinInfo& info)
{
    Packet packet;
    struct sockaddr_in client_addr;
    socklen_t slen = sizeof(client_addr);

    std::cout << "Waiting for client connection...\n\n";

    while (1)
    {

        int receiver = recvfrom(sockfd, packet.buffer, PACKET_SIZE, 0, (struct sockaddr*)&client_addr, &slen);
        if (receiver < 0)
        {          
            if (errno == EWOULDBLOCK)
            {
                errno = 0;
                continue;
            }
            else
            {
                std::cerr << "Error: Could not receive from client\n\n";
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }
        else if (receiver == 0)
        {
            continue;
        }
        else
        {
            if (packet.type() == GET)
            {
                std::string filename = packet_string(packet);

                if (filename.empty())
                {
                    std::cout << "Warning: Received invalid filename request: Discarding\n\n";
                }
                else if (filename == SUCCESS_MSG)
                {
                    std::cout << "Warning: Received success message: Discarding\n\n";
                }
                else
                {
                    std::cout << "Received GET request from client\n\n";
                    vector<Packet> packets;
                    vector<Timer> timers;
                    if (!send_file(filename, sockfd, client_addr, info))
                    {
                        std::cout << "ERROR: Client stopped responding. Ending connection...\n\n";
                        return;
                    }
                    receive_success_msg(sockfd, client_addr);

                    std::cout << "Waiting for client connection...\n\n";
                }
            }
        }
    }
}

void parse_file(std::string& filename, vector<Packet>& packets, vector<Timer>& timers)
{
    FILE *infile;
    infile = fopen(filename.c_str(), "rb");
    if (infile == NULL)
    {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        return;
    }

    int current_seq = 0;
    size_t numread = 0;

    while(!feof(infile))
    {
        char data[PACKET_SIZE - HEADER_SIZE];
        bzero(data, PACKET_SIZE - HEADER_SIZE);
        numread = fread(data, 1, PACKET_SIZE - HEADER_SIZE, infile);
        if (numread != PACKET_SIZE - HEADER_SIZE && !feof(infile))
        {
            fclose(infile);
            std::cerr << "Error: Could not properly read file: " << filename << std::endl;
            exit(EXIT_FAILURE);
        }

        Packet packet = Packet(data, numread, current_seq, TRN);
        Timer timer = Timer();
        packets.push_back(packet);
        timers.push_back(timer);
        current_seq = (current_seq + 1) % SEQ_NUM;
    }

    Packet final_packet = Packet(current_seq, TRN);
    Timer final_timer = Timer();
    packets.push_back(final_packet);
    timers.push_back(final_timer);

    fclose(infile);
}

bool send_file(std::string filename, int sockfd, struct sockaddr_in client_addr, GremlinInfo& info)
{
    socklen_t slen = sizeof(client_addr);

    vector<Packet> packets;
    vector<Timer> timers;
    parse_file(filename, packets, timers);

    int window_end = packets.size();
    int window_base = 0;
    int current = 0;
    int timeout_counter = 0;

    vector<Packet> delay_packets;
    vector<Timer> delay_timers;

    while (window_base < window_end)
    {
        // Check on the delayed packets
        if (!delay_timers.empty())
        {
            for (int i = 0; i < delay_timers.size(); ++i)
            {
                if (delay_timers[i].timeout(info.delay_amount_ms))
                {
                    std::cout << "SENDING: sequence " << (int)delay_packets[i].sequence() << "\n";
                    std::cout << "DATA:\n";
                    std::cout << packet_string(delay_packets[i], 48);
                    std::cout << "\n\n";
                    if (sendto(sockfd, delay_packets[i].buffer, PACKET_SIZE, 0, (struct sockaddr*) &client_addr, slen) == -1)
                    {
                        std::cerr << "Error: could not send packet to client" << std::endl;
                        close(sockfd);
                        exit(EXIT_FAILURE);
                    }

                    delay_timers.erase(delay_timers.begin() + i);
                    delay_packets.erase(delay_packets.begin() + i);
                    break;
                }
            }
        }
        else if (current < (window_base + WINDOW_SIZE) && current < window_end)
        {
            Packet temp = packets[current];
            int result = send_packet(sockfd, client_addr, temp, info);
            if (result == DELAYED)
            {
                Timer delay_timer = Timer();
                delay_timer.start();
                delay_timers.push_back(delay_timer);
                delay_packets.push_back(temp);
            }
            timers[current].start();
            current++;
        }
        else if (timers[window_base].timeout(SERVER_TIMEOUT_MSEC))
        {
            std::cout << "TIMEOUT: Retransmitting current window\n\n";
            current = window_base;
            timeout_counter++;
            if (timeout_counter > SERVER_CANCEL_TIMEOUT_COUNT)
            {
                return false;
            }
        }

        Packet received;
        struct sockaddr_in client_rec;
        socklen_t slen_rec = sizeof(client_addr);

        int receiver = recvfrom(sockfd, received.buffer, PACKET_SIZE, 0, (struct sockaddr*)&client_rec, &slen_rec);
        if (receiver < 0)
        {          
            if (errno == EWOULDBLOCK)
            {
                errno = 0;
            }
            else
            {
                std::cerr << "Error: Could not receive from client" << std::endl;
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }
        else if (receiver > 0)
        {
            timeout_counter = 0;

            if (received.type() == ACK)
            {
                std::cout << "ACKNOLEDGE: sequence " << (int) received.sequence() << "\n\n";
                if (received.sequence() == ((window_base + 1) % SEQ_NUM))
                {
                    window_base++;
                    if (current < window_base)
                        current = window_base;
                }             
            }
            else if (received.type() == NAK)
            {
                std::cout << "DAMAGED DATA: sequence " << (int) received.sequence() << "\n\n";
                current = window_base;
            }
        }
    }

    return true;
}

int send_packet(int sockfd, struct sockaddr_in client_addr, Packet& packet, GremlinInfo& info)
{
    int result = gremlin(packet.data(), info.corrupt_chance, info.loss_chance, info.delay_chance);
    if (result == FINE)
    {
        std::cout << "SENDING: sequence " << (int)packet.sequence() << "\n";
        std::cout << "DATA:\n";
        std::cout << packet_string(packet, 48);
        std::cout << "\n\n";
        if (sendto(sockfd, packet.buffer, PACKET_SIZE, 0, (struct sockaddr*) &client_addr, sizeof(client_addr)) == -1)
        {
            std::cerr << "Error: could not send packet to client" << std::endl;
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    return result;
}

void receive_success_msg(int sockfd, struct sockaddr_in client_addr)
{
    Packet received;
    struct sockaddr_in client_rec;
    socklen_t slen_rec = sizeof(client_addr);
    Timer final_timer = Timer();
    final_timer.start();

    while (1)
    {
        if (final_timer.timeout(SERVER_CLOSE_TIMEOUT_MSEC))
        {
            break;
        }
        int receiver = recvfrom(sockfd, received.buffer, PACKET_SIZE, 0, (struct sockaddr*)&client_rec, &slen_rec);
        if (receiver < 0)
        {          
            if (errno == EWOULDBLOCK)
            {
                errno = 0;
            }
            else
            {
                std::cerr << "Error: Could not receive from client" << std::endl;
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }
        else if (receiver > 0)
        {
            if (received.type() == GET)
            {
                break;
            }
        }
    }

    std::cout << "FINISHED: Successful GET command completed" << std::endl << std::endl;
}

int main(int argc, char** argv)
{
	if (argc != 5)
    {
        std::cout << "Usage: " << argv[0] << " ";
        std::cout << "<corrupt %%> <loss %%> <delay %%> <delay-amount-ms>" << std::endl;
        exit(EXIT_FAILURE);
    }

    GremlinInfo gremlin_info;

    gremlin_info.corrupt_chance = atoi(argv[1]);
    gremlin_info.loss_chance = atoi(argv[2]);
    gremlin_info.delay_chance = atoi(argv[3]);
    gremlin_info.delay_amount_ms = atoi(argv[4]);

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
    int sockfd = -1; 

    socklen_t slen = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
    	perror("Error: Could not create socket\n");
    	exit(EXIT_FAILURE);
    }

    // Set the receiving function to non-blocking
    int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1)
    {
    	fprintf(stderr, "Error: Could not bind to port\n");
    	close(sockfd);
    	exit(EXIT_FAILURE);
    }

    printf("Successfully bound server to port %d and listening for clients...\n\n", SERVER_PORT);
	
	receive_commands(sockfd, gremlin_info); // include params for gremlin?
 
    close(sockfd);
    exit(EXIT_SUCCESS);
}

int gremlin(char *data, int corrupt_chance, int loss_chance, int delay_chance)
{
    if (corrupt_chance < 0)
        corrupt_chance = 0;
    if (corrupt_chance > 100)
        corrupt_chance = 100;
    if (loss_chance < 0)
        loss_chance = 0;
    if (loss_chance > 100)
        loss_chance = 100;
    if (delay_chance < 0)
        delay_chance = 0;
    if (delay_chance > 100)
        delay_chance = 100;
    int corrupt_roll = rand() % 101;
    int loss_roll = rand() % 101;
    int delay_roll = rand() % 101;
    if (loss_roll <= loss_chance) 
    {
        return LOST;
    }
    if (corrupt_roll <= corrupt_chance)
    {
        int num_corrupt = rand() % 101;
        if (num_corrupt <= 70)
        {
            int corrupt_byte = rand() % (PACKET_SIZE - HEADER_SIZE);
            data[corrupt_byte] = ~data[corrupt_byte];
        }
        else if (num_corrupt <= 90)
        {
            int corrupt_byte = rand() % (PACKET_SIZE - HEADER_SIZE);
            data[corrupt_byte] = ~data[corrupt_byte];

            int prev_corrupt = corrupt_byte;
            while (prev_corrupt == corrupt_byte)
            {
                corrupt_byte = rand() % (PACKET_SIZE - HEADER_SIZE);
            }
            data[corrupt_byte] = ~data[corrupt_byte];
        }
        else
        {
            int corrupt_byte = rand() % (PACKET_SIZE - HEADER_SIZE);
            data[corrupt_byte] = ~data[corrupt_byte];

            int prev_corrupt1 = corrupt_byte;
            while (prev_corrupt1 == corrupt_byte)
            {
                corrupt_byte = rand() % (PACKET_SIZE - HEADER_SIZE);
            }
            data[corrupt_byte] = ~data[corrupt_byte];

            int prev_corrupt2 = corrupt_byte;
            while (prev_corrupt1 == corrupt_byte
                || prev_corrupt2 == corrupt_byte)
            {
                corrupt_byte = rand() % (PACKET_SIZE - HEADER_SIZE);
            }
            data[corrupt_byte] = ~data[corrupt_byte];
        }
    }
    if (delay_roll <= delay_chance)
    {
        return DELAYED;
    }
    return FINE;
}