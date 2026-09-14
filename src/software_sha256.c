#include "software_sha256.h"

#include <stddef.h>
#include <string.h>

#include "pico.h"

static const uint32_t sha256_initial_state[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
};

static const uint32_t __not_in_flash("software_sha256_constants")
    sha256_round_constants[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

static inline uint32_t load_be32(const uint8_t *bytes) {
    uint32_t value;
    memcpy(&value, bytes, sizeof(value));
    return __builtin_bswap32(value);
}

static inline uint32_t rotate_right(uint32_t value, unsigned shift) {
    return (value >> shift) | (value << (32u - shift));
}

static __attribute__((optimize("unroll-loops"))) void
__not_in_flash_func(software_sha256_compress)(uint32_t state[8],
                                              const uint32_t block[16]) {
    uint32_t schedule[64];
    memcpy(schedule, block, 16u * sizeof(schedule[0]));
    for (unsigned word = 16u; word < 64u; ++word) {
        const uint32_t x = schedule[word - 15u];
        const uint32_t y = schedule[word - 2u];
        const uint32_t sigma0 = rotate_right(x, 7u)
                                ^ rotate_right(x, 18u) ^ (x >> 3u);
        const uint32_t sigma1 = rotate_right(y, 17u)
                                ^ rotate_right(y, 19u) ^ (y >> 10u);
        schedule[word] = schedule[word - 16u] + schedule[word - 7u]
                         + sigma0 + sigma1;
    }

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    for (unsigned round = 0u; round < 64u; ++round) {
        const uint32_t word = schedule[round];
        const uint32_t sum1 = rotate_right(e, 6u) ^ rotate_right(e, 11u)
                              ^ rotate_right(e, 25u);
        const uint32_t choice = g ^ (e & (f ^ g));
        const uint32_t temp1 = h + sum1 + choice
                               + sha256_round_constants[round] + word;
        const uint32_t sum0 = rotate_right(a, 2u) ^ rotate_right(a, 13u)
                              ^ rotate_right(a, 22u);
        const uint32_t majority = (a & b) | (c & (a | b));
        const uint32_t temp2 = sum0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

void software_bitcoin_hasher_begin(software_bitcoin_hasher_t *hasher,
                                   const uint8_t header[80]) {
    uint32_t first_block[16];
    for (size_t word = 0u; word < 16u; ++word) {
        first_block[word] = load_be32(&header[word * 4u]);
    }
    memcpy(hasher->midstate, sha256_initial_state, sizeof(hasher->midstate));
    software_sha256_compress(hasher->midstate, first_block);
    for (size_t word = 0u; word < 3u; ++word) {
        hasher->tail_words[word] = load_be32(&header[(16u + word) * 4u]);
    }
}

void software_bitcoin_hash_nonce(const software_bitcoin_hasher_t *hasher,
                                 uint32_t nonce,
                                 uint8_t hash[32]) {
    uint32_t block[16] = {0};
    uint32_t first_digest[8];
    memcpy(first_digest, hasher->midstate, sizeof(first_digest));
    block[0] = hasher->tail_words[0];
    block[1] = hasher->tail_words[1];
    block[2] = hasher->tail_words[2];
    block[3] = __builtin_bswap32(nonce);
    block[4] = 0x80000000u;
    block[15] = 80u * 8u;
    software_sha256_compress(first_digest, block);

    memset(block, 0, sizeof(block));
    memcpy(block, first_digest, sizeof(first_digest));
    block[8] = 0x80000000u;
    block[15] = 32u * 8u;
    uint32_t second_digest[8];
    memcpy(second_digest, sha256_initial_state, sizeof(second_digest));
    software_sha256_compress(second_digest, block);

    for (size_t word = 0u; word < 8u; ++word) {
        const uint32_t encoded = __builtin_bswap32(second_digest[word]);
        memcpy(&hash[word * 4u], &encoded, sizeof(encoded));
    }
}
