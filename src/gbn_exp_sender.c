#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "../include/channel.h"

#define DATA_SIZE 512
#define WINDOW_SIZE 4
#define MAX_PACKETS 1000
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
    static Packet packets[MAX_PACKETS];

    int sockfd;
    int total_packets = 0;
    int base = 0;
    int next_seq = 0;

    int total_attempts = 0;
    int retransmissions = 0;
    long file_bytes = 0;

    FILE *file = fopen("test_data/experiment_input.txt", "rb");

    if (file == NULL) {
        printf("Could not open experiment_input.txt\n");
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

        file_bytes += packet.data_len;

        if (packet.data_len < DATA_SIZE) {
            packet.is_last = 1;
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

    srand((unsigned int)time(NULL) ^ getpid());

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        printf("Socket creation failed\n");
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

    struct sockaddr_in receiver_addr;
    socklen_t addr_len = sizeof(receiver_addr);

    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(8080);
    receiver_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    struct timeval start_time;
    struct timeval end_time;

    gettimeofday(&start_time, NULL);

    while (base < total_packets) {

        int window_start = next_seq;

        while (
            next_seq < base + WINDOW_SIZE &&
            next_seq < total_packets
        ) {

            total_attempts++;

            if (should_drop(PACKET_LOSS)) {

                printf(
                    "Packet %d dropped by emulator\n",
                    next_seq
                );
            }
            else {

                sendto(
                    sockfd,
                    &packets[next_seq],
                    sizeof(Packet),
                    0,
                    (struct sockaddr *)&receiver_addr,
                    sizeof(receiver_addr)
                );
            }

            next_seq++;
        }

        if (window_start < next_seq) {
            printf(
                "Sent window: packets %d to %d\n",
                window_start,
                next_seq - 1
            );
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

        if (n > 0) {

            if (
                ack >= base &&
                ack < total_packets
            ) {

                printf("Received cumulative ACK%d\n", ack);

                base = ack + 1;
            }
        }
        else {

            printf(
                "Timeout at base %d\n",
                base
            );

            printf(
                "Retransmitting packets %d to %d\n",
                base,
                next_seq - 1
            );

            for (int i = base; i < next_seq; i++) {

                retransmissions++;
                total_attempts++;

                if (should_drop(PACKET_LOSS)) {

                    printf(
                        "Retransmitted packet %d dropped by emulator\n",
                        i
                    );
                }
                else {

                    sendto(
                        sockfd,
                        &packets[i],
                        sizeof(Packet),
                        0,
                        (struct sockaddr *)&receiver_addr,
                        sizeof(receiver_addr)
                    );
                }
            }
        }
    }

    gettimeofday(&end_time, NULL);

    double transfer_time =
        time_difference(start_time, end_time);

    double goodput =
        (file_bytes * 8.0) /
        transfer_time /
        1000.0;

    close(sockfd);

    printf("\n--- Go-Back-N Results ---\n");
    printf("Packet loss rate : %.0f%%\n", PACKET_LOSS * 100);
    printf("File size        : %ld bytes\n", file_bytes);
    printf("Data packets     : %d\n", total_packets);
    printf("Window size      : %d\n", WINDOW_SIZE);
    printf("Send attempts    : %d\n", total_attempts);
    printf("Retransmissions  : %d\n", retransmissions);
    printf("Transfer time    : %.3f seconds\n", transfer_time);
    printf("Goodput          : %.3f kbps\n", goodput);

    return 0;
}
