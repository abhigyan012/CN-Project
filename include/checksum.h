#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include <stddef.h>
#include "packet.h"

/*
 * Calculates standard 16-bit Internet Checksum (RFC 1071) over a byte buffer.
 */
uint16_t compute_checksum(const void *buffer, size_t length);

/*
 * Zeros out pkt->checksum, computes the checksum over header + data,
 * and writes the result into pkt->checksum.
 */
void packet_set_checksum(Packet *pkt);

/*
 * Verifies if the packet's checksum matches its contents.
 * Returns 1 if valid (not corrupted), 0 if corrupted.
 */
int packet_is_valid(const Packet *pkt);

#endif /* CHECKSUM_H */
