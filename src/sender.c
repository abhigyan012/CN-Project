#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include "../include/channel.h"

#define DATA_SIZE 512
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
    int seq = 0;
    int ack;

    Packet packet;

    struct sockaddr_in receiver_addr;
    socklen_t addr_len = sizeof(receiver_addr);

    FILE *file = fopen("test_data/input.txt", "rb");

    if (file == NULL) {
        printf("Could not open input.txt\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        printf("Socket creation failed.\n");
        fclose(file);
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

    while (1) {

        memset(&packet, 0, sizeof(packet));

        packet.seq = seq;

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

        int ack_received = 0;

        while (!ack_received) {

            if (should_drop(PACKET_LOSS)) {

                printf(
                    "Channel emulator: packet %d dropped.\n",
                    packet.seq
                );

            }
            else {

                Packet send_packet = packet;

                if (
                    should_corrupt(CORRUPTION_RATE) &&
                    send_packet.data_len > 0
                ) {

                    send_packet.data[0] ^= 1;

                    printf(
                        "Channel emulator: packet %d corrupted.\n",
                        packet.seq
                    );
                }

                sendto(
                    sockfd,
                    &send_packet,
                    sizeof(send_packet),
                    0,
                    (struct sockaddr *)&receiver_addr,
                    sizeof(receiver_addr)
                );

                printf(
                    "Sent packet %d (%d bytes, checksum = %u)\n",
                    packet.seq,
                    packet.data_len,
                    packet.checksum
                );
            }

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
                    "Timeout! Resending packet %d...\n\n",
                    packet.seq
                );
            }
            else if (ack == packet.seq) {

                printf("Received ACK%d\n\n", ack);

                ack_received = 1;

                seq = 1 - seq;
            }
        }

        if (packet.is_last) {
            break;
        }
    }

    fclose(file);
    close(sockfd);

    printf("File transfer completed.\n");

    return 0;
}
