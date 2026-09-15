#ifndef PICO2_MINER_SOFTWARE_SHA256_H
#define PICO2_MINER_SOFTWARE_SHA256_H

#include <stdint.h>

typedef struct software_bitcoin_hasher {
    uint32_t midstate[8];
    uint32_t tail_round3_state[8];
    uint32_t tail_words[3];
    uint32_t tail_round3_temp1_base;
    uint32_t tail_round3_temp2;
    uint32_t tail_schedule16;
    uint32_t tail_schedule17;
    uint32_t tail_schedule18_base;
    uint32_t tail_schedule19_base;
} software_bitcoin_hasher_t;

void software_bitcoin_hasher_begin(software_bitcoin_hasher_t *hasher,
                                   const uint8_t header[80]);
void software_bitcoin_hash_nonce(const software_bitcoin_hasher_t *hasher,
                                 uint32_t nonce,
                                 uint8_t hash[32]);
uint32_t software_bitcoin_hash_nonce_high_word(
    const software_bitcoin_hasher_t *hasher, uint32_t nonce);

#endif
