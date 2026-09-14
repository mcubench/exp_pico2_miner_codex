#ifndef PICO2_MINER_SOFTWARE_SHA256_H
#define PICO2_MINER_SOFTWARE_SHA256_H

#include <stdint.h>

typedef struct software_bitcoin_hasher {
    uint32_t midstate[8];
    uint32_t tail_words[3];
} software_bitcoin_hasher_t;

void software_bitcoin_hasher_begin(software_bitcoin_hasher_t *hasher,
                                   const uint8_t header[80]);
void software_bitcoin_hash_nonce(const software_bitcoin_hasher_t *hasher,
                                 uint32_t nonce,
                                 uint8_t hash[32]);

#endif
