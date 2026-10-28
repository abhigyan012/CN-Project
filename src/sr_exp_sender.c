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
#define RETRANSMIT_TIMEOUT 1
#define PACKET_LOSS 0.20
#define FIN_SEQ -1

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
    static int sent[MAX_PACKETS];
    static int acked[MAX_PACKETS];
    static time_t last_sent[MAX_PACKETS];

    int sockfd;
    int total_packets = 0;
    int base = 0;

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

    struct timeval socket_timeout;
    socket_timeout.tv_sec = 0;
    socket_timeout.tv_usec = 100000;

    setsockopt(
        sockfd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &socket_timeout,
        sizeof(socket_timeout)
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

        int end = base + WINDOW_SIZE;

        if (end > total_packets) {
            end = total_packets;
        }

        int first_new = -1;
        int last_new = -1;

        for (int i = base; i < end; i++) {

            if (!sent[i]) {

                total_attempts++;

                if (first_new == -1) {
                    first_new = i;
                }

                last_new = i;

                if (should_drop(PACKET_LOSS)) {

                    printf(
                        "Packet %d dropped by emulator\n",
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

                sent[i] = 1;
                last_sent[i] = time(NULL);
            }
        }

        if (first_new != -1) {

            printf(
                "Sent window packets %d to %d\n",
                first_new,
                last_new
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

        if (
            n > 0 &&
            ack >= 0 &&
            ack < total_packets
        ) {

            if (!acked[ack]) {
                acked[ack] = 1;
            }
        }

        time_t now = time(NULL);

        for (int i = base; i < end; i++) {

            if (
                sent[i] &&
                !acked[i] &&
                difftime(now, last_sent[i]) >= RETRANSMIT_TIMEOUT
            ) {

                retransmissions++;
                total_attempts++;

                printf(
                    "Timeout for packet %d. Retransmitting...\n",
                    i
                );

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

                last_sent[i] = time(NULL);
            }
        }

        int old_base = base;

        while (
            base < total_packets &&
            acked[base]
        ) {
            base++;
        }

        if (base != old_base) {

            printf(
                "Window moved. New base = %d\n",
                base
            );
        }
    }

    Packet fin_packet;
    memset(&fin_packet, 0, sizeof(fin_packet));
    fin_packet.seq = FIN_SEQ;

    int fin_ack_received = 0;

    while (!fin_ack_received) {

        sendto(
            sockfd,
            &fin_packet,
            sizeof(fin_packet),
            0,
            (struct sockaddr *)&receiver_addr,
            sizeof(receiver_addr)
        );

        int ack;

        int n = recvfrom(
            sockfd,
            &ack,
            sizeof(ack),
            0,
            (struct sockaddr *)&receiver_addr,
            &addr_len
        );

        if (n > 0 && ack == FIN_SEQ) {
            fin_ack_received = 1;
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

    printf("\n--- Selective Repeat Results ---\n");
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
