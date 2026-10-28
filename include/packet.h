#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

/* Maximum size of actual file data carried in one packet (1 KB) */
#define MAX_PAYLOAD_SIZE 1024

/* Types of packets we exchange */
typedef enum {
    PKT_DATA = 1,   /* Contains a piece of the file */
    PKT_ACK  = 2,   /* Acknowledgment from receiver */
    PKT_FIN  = 3    /* Signals that file transfer is finished */
} PacketType;

/*
 * The Packet structure (our envelope).
 * #pragma pack(push, 1) prevents the compiler from adding hidden padding bytes,
 * guaranteeing the exact same binary layout across sender and receiver.
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  type;                        /* 1 byte: PKT_DATA, PKT_ACK, or PKT_FIN */
    uint32_t seq_num;                     /* 4 bytes: Sequence number */
    uint32_t ack_num;                     /* 4 bytes: Acknowledged sequence number */
    uint16_t checksum;                    /* 2 bytes: Internet Checksum (RFC 1071) */
    uint16_t data_len;                    /* 2 bytes: Actual bytes of data in payload */
    char     payload[MAX_PAYLOAD_SIZE];   /* File chunk */
} Packet;
#pragma pack(pop)

#endif /* PACKET_H */
