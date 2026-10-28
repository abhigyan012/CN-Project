#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include "../include/channel.h"

#define DATA_SIZE 512
#define WINDOW_SIZE 4
#define MAX_PACKETS 1000
#define PACKET_LOSS 0.20
#define CORRUPTION_RATE 0.10

typedef struct {
    int seq;
    int data_len;
    int is_last;
    unsigned int checksum;
    char data[DATA_SIZE];
} Packet;

unsigned int calculate_checksum(char *data, int len) {
    unsigned int sum = 0;

    for (int i = 0; i < len; i++) {
        sum += (unsigned char)data[i];
    }

    return sum;
}

int main() {
    int sockfd;
    Packet packets[MAX_PACKETS];
    int total_packets = 0;

    struct sockaddr_in receiver_addr;
    socklen_t addr_len = sizeof(receiver_addr);

    FILE *file = fopen("test_data/input.txt", "rb");

    if (file == NULL) {
        printf("Could not open input.txt\n");
        return 1;
    }

    while (1) {
        Packet packet;

        memset(&packet, 0, sizeof(packet));

        packet.seq = total_packets;

        packet.data_len = fread(
            packet.data,
            1,
            DATA_SIZE,
            file
        );

        if (packet.data_len < DATA_SIZE) {
            packet.is_last = 1;
        }
        else {
            packet.is_last = 0;
        }

        packet.checksum = calculate_checksum(
            packet.data,
            packet.data_len
        );

        packets[total_packets] = packet;
        total_packets++;

        if (packet.is_last) {
            break;
        }
    }

    fclose(file);

    printf("Total packets: %d\n", total_packets);
    printf("Window size: %d\n", WINDOW_SIZE);
    printf("Packet loss: %.0f%%\n", PACKET_LOSS * 100);
    printf("Corruption rate: %.0f%%\n\n", CORRUPTION_RATE * 100);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        printf("Socket creation failed.\n");
        return 1;
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    setsockopt(
        sockfd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)
    );

    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(8080);
    receiver_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int base = 0;
    int next_seq = 0;

    while (base < total_packets) {

        while (
            next_seq < base + WINDOW_SIZE &&
            next_seq < total_packets
        ) {

            if (should_drop(PACKET_LOSS)) {

                printf(
                    "Channel emulator: packet %d dropped.\n",
                    packets[next_seq].seq
                );
            }
            else {

                Packet send_packet = packets[next_seq];

                if (
                    should_corrupt(CORRUPTION_RATE) &&
                    send_packet.data_len > 0
                ) {

                    send_packet.data[0] ^= 1;

                    printf(
                        "Channel emulator: packet %d corrupted.\n",
                        send_packet.seq
                    );
                }

                sendto(
                    sockfd,
                    &send_packet,
                    sizeof(Packet),
                    0,
                    (struct sockaddr *)&receiver_addr,
                    sizeof(receiver_addr)
                );

                printf(
                    "Sent packet %d (%d bytes)\n",
                    send_packet.seq,
                    send_packet.data_len
                );
            }

            next_seq++;
        }

        int ack;

        int n = recvfrom(
            sockfd,
            &ack,
            sizeof(ack),
            0,
            (struct sockaddr *)&receiver_addr,
            &addr_len
        );

        if (n < 0) {

            printf(
                "\nTimeout! Resending from packet %d\n\n",
                base
            );

            next_seq = base;
        }
        else {

            printf("Received ACK%d\n", ack);

            if (ack >= base) {

                base = ack + 1;

                printf(
                    "Window moved. New base = %d\n\n",
                    base
                );
            }
        }
    }

    close(sockfd);

    printf("Go-Back-N file transfer completed.\n");

    return 0;
}
