#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "../include/channel.h"

#define DATA_SIZE 512
#define PACKET_LOSS 0.20

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

double time_difference(struct timeval start, struct timeval end) {
    return
        (end.tv_sec - start.tv_sec) +
        (end.tv_usec - start.tv_usec) / 1000000.0;
}

int main() {
    int sockfd;
    int seq = 0;
    int ack;

    long file_bytes = 0;
    int original_packets = 0;
    int total_attempts = 0;
    int retransmissions = 0;

    Packet packet;

    struct sockaddr_in receiver_addr;
    socklen_t addr_len = sizeof(receiver_addr);

    FILE *file = fopen("test_data/experiment_input.txt", "rb");

    if (file == NULL) {
        printf("Could not open experiment_input.txt\n");
        return 1;
    }

    srand((unsigned int)time(NULL) ^ getpid());

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        printf("Socket creation failed.\n");
        fclose(file);
        return 1;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
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
    receiver_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    struct timeval start_time;
    struct timeval end_time;

    gettimeofday(&start_time, NULL);

    while (1) {

        memset(&packet, 0, sizeof(packet));

        packet.seq = seq;

        packet.data_len = fread(
            packet.data,
            1,
            DATA_SIZE,
            file
        );

        file_bytes += packet.data_len;
        original_packets++;

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
        int attempt = 0;

        while (!ack_received) {

            attempt++;
            total_attempts++;

            if (attempt > 1) {
                retransmissions++;
            }

            if (should_drop(PACKET_LOSS)) {

                printf(
                    "Packet %d dropped by emulator\n",
                    packet.seq
                );
            }
            else {

                sendto(
                    sockfd,
                    &packet,
                    sizeof(packet),
                    0,
                    (struct sockaddr *)&receiver_addr,
                    sizeof(receiver_addr)
                );

                printf(
                    "Sent packet %d (%d bytes)\n",
                    packet.seq,
                    packet.data_len
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
                    "Timeout! Retransmitting packet %d\n",
                    packet.seq
                );
            }
            else if (ack == packet.seq) {

                ack_received = 1;
                seq = 1 - seq;
            }
        }

        if (packet.is_last) {
            break;
        }
    }

    gettimeofday(&end_time, NULL);

    double transfer_time =
        time_difference(start_time, end_time);

    double goodput =
        (file_bytes * 8.0) /
        transfer_time /
        1000.0;

    fclose(file);
    close(sockfd);

    printf("\n--- Stop-and-Wait Results ---\n");
    printf("Packet loss rate : %.0f%%\n", PACKET_LOSS * 100);
    printf("File size        : %ld bytes\n", file_bytes);
    printf("Data packets     : %d\n", original_packets);
    printf("Send attempts    : %d\n", total_attempts);
    printf("Retransmissions  : %d\n", retransmissions);
    printf("Transfer time    : %.3f seconds\n", transfer_time);
    printf("Goodput          : %.3f kbps\n", goodput);

    return 0;
}
