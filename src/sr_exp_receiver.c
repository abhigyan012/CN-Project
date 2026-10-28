#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DATA_SIZE 512
#define WINDOW_SIZE 4
#define MAX_PACKETS 1000
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
    int base = 0;

    static Packet packet_buffer[MAX_PACKETS];
    static int received[MAX_PACKETS];

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    socklen_t client_len = sizeof(client_addr);

    FILE *file = fopen("outputs/received_sr_exp.txt", "wb");

    if (file == NULL) {
        printf("Could not create output file\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        printf("Socket creation failed\n");
        fclose(file);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(
        sockfd,
        (struct sockaddr *)&server_addr,
        sizeof(server_addr)
    ) < 0) {

        printf("Bind failed\n");

        fclose(file);
        close(sockfd);

        return 1;
    }

    printf("Selective Repeat experiment receiver ready...\n");

    while (1) {

        Packet packet;

        int n = recvfrom(
            sockfd,
            &packet,
            sizeof(Packet),
            0,
            (struct sockaddr *)&client_addr,
            &client_len
        );

        if (n < 0) {
            continue;
        }

        if (packet.seq == FIN_SEQ) {

            int fin_ack = FIN_SEQ;

            sendto(
                sockfd,
                &fin_ack,
                sizeof(fin_ack),
                0,
                (struct sockaddr *)&client_addr,
                client_len
            );

            break;
        }

        if (
            packet.seq < 0 ||
            packet.seq >= MAX_PACKETS
        ) {
            continue;
        }

        unsigned int checksum =
            calculate_checksum(
                packet.data,
                packet.data_len
            );

        if (checksum != packet.checksum) {
            continue;
        }

        if (packet.seq < base) {

            int ack = packet.seq;

            sendto(
                sockfd,
                &ack,
                sizeof(ack),
                0,
                (struct sockaddr *)&client_addr,
                client_len
            );

            continue;
        }

        if (
            packet.seq >= base &&
            packet.seq < base + WINDOW_SIZE
        ) {

            if (!received[packet.seq]) {

                packet_buffer[packet.seq] = packet;
                received[packet.seq] = 1;
            }

            int ack = packet.seq;

            sendto(
                sockfd,
                &ack,
                sizeof(ack),
                0,
                (struct sockaddr *)&client_addr,
                client_len
            );

            while (
                base < MAX_PACKETS &&
                received[base]
            ) {

                fwrite(
                    packet_buffer[base].data,
                    1,
                    packet_buffer[base].data_len,
                    file
                );

                fflush(file);

                base++;
            }
        }
    }

    fclose(file);
    close(sockfd);

    printf("SR experiment transfer received.\n");

    return 0;
}
