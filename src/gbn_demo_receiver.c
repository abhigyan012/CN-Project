#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "../include/channel.h"

#define DATA_SIZE 512
#define ACK_LOSS 0.20

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
    int expected_seq = 0;

    Packet packet;

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    FILE *file = fopen("outputs/received_gbn.txt", "wb");

    if (file == NULL) {
        printf("Could not create received_gbn.txt\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        printf("Socket creation failed.\n");
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

        printf("Bind failed.\n");

        fclose(file);
        close(sockfd);

        return 1;
    }

    printf("Go-Back-N receiver waiting...\n\n");

    while (1) {

        int n = recvfrom(
            sockfd,
            &packet,
            sizeof(packet),
            0,
            (struct sockaddr *)&client_addr,
            &client_len
        );

        if (n < 0) {
            continue;
        }

        unsigned int calculated_checksum =
            calculate_checksum(
                packet.data,
                packet.data_len
            );

        if (calculated_checksum != packet.checksum) {

            printf(
                "Corrupted packet %d discarded.\n",
                packet.seq
            );

            continue;
        }

        if (packet.seq == expected_seq) {

            fwrite(
                packet.data,
                1,
                packet.data_len,
                file
            );

            fflush(file);

            printf(
                "Accepted packet %d (%d bytes)\n",
                packet.seq,
                packet.data_len
            );

            expected_seq++;
        }
        else {

            printf(
                "Out-of-order packet %d discarded. Expected %d\n",
                packet.seq,
                expected_seq
            );
        }

        int ack = expected_seq - 1;

        if (should_drop(ACK_LOSS)) {

            printf(
                "Channel emulator: cumulative ACK%d dropped.\n\n",
                ack
            );
        }
        else {

            sendto(
                sockfd,
                &ack,
                sizeof(ack),
                0,
                (struct sockaddr *)&client_addr,
                client_len
            );

            printf(
                "Sent cumulative ACK%d\n\n",
                ack
            );
        }

        if (
            packet.is_last &&
            packet.seq == expected_seq - 1
        ) {
            break;
        }
    }

    fclose(file);
    close(sockfd);

    printf(
        "Go-Back-N file received successfully.\n"
    );

    return 0;
}
