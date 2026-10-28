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
#define RETRANSMIT_TIMEOUT 5
#define PACKET_LOSS 0.20
#define CORRUPTION_RATE 0.10
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

int main() {
    int sockfd;

    static Packet packets[MAX_PACKETS];
    static int sent[MAX_PACKETS];
    static int acked[MAX_PACKETS];
    static time_t last_sent[MAX_PACKETS];

    int total_packets = 0;

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

    struct timeval wait_time;
    wait_time.tv_sec = 1;
    wait_time.tv_usec = 0;

    setsockopt(
        sockfd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &wait_time,
        sizeof(wait_time)
    );

    struct sockaddr_in receiver_addr;
    socklen_t addr_len = sizeof(receiver_addr);

    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(8080);
    receiver_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    int base = 0;

    while (base < total_packets) {

        int end = base + WINDOW_SIZE;

        if (end > total_packets) {
            end = total_packets;
        }

        for (int i = base; i < end; i++) {

            if (!sent[i]) {

                if (should_drop(PACKET_LOSS)) {

                    printf(
                        "Channel emulator: packet %d dropped.\n",
                        i
                    );
                }
                else {

                    Packet send_packet = packets[i];

                    if (
                        should_corrupt(CORRUPTION_RATE) &&
                        send_packet.data_len > 0
                    ) {

                        send_packet.data[0] ^= 1;

                        printf(
                            "Channel emulator: packet %d corrupted.\n",
                            i
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
                        i,
                        send_packet.data_len
                    );
                }

                sent[i] = 1;
                last_sent[i] = time(NULL);
            }
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
                printf("Received ACK%d\n", ack);
            }
        }

        time_t now = time(NULL);

        for (int i = base; i < end; i++) {

            if (
                sent[i] &&
                !acked[i] &&
                difftime(now, last_sent[i]) >= RETRANSMIT_TIMEOUT
            ) {

                printf(
                    "Timeout for packet %d. Retransmitting...\n",
                    i
                );

                if (should_drop(PACKET_LOSS)) {

                    printf(
                        "Channel emulator: retransmitted packet %d dropped.\n",
                        i
                    );
                }
                else {

                    Packet resend_packet = packets[i];

                    if (
                        should_corrupt(CORRUPTION_RATE) &&
                        resend_packet.data_len > 0
                    ) {

                        resend_packet.data[0] ^= 1;

                        printf(
                            "Channel emulator: retransmitted packet %d corrupted.\n",
                            i
                        );
                    }

                    sendto(
                        sockfd,
                        &resend_packet,
                        sizeof(Packet),
                        0,
                        (struct sockaddr *)&receiver_addr,
                        sizeof(receiver_addr)
                    );

                    printf(
                        "Retransmitted packet %d\n",
                        i
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
                "Window moved. New base = %d\n\n",
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

        printf("FIN sent. Waiting for FIN_ACK...\n");

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
            printf("FIN_ACK received.\n");
            fin_ack_received = 1;
        }
        else {
            printf("FIN_ACK timeout. Sending FIN again...\n");
        }
    }

    close(sockfd);

    printf("Selective Repeat file transfer completed.\n");

    return 0;
}
