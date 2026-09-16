#include "software_sha256.h"

#include <stddef.h>
#include <string.h>

#include "pico.h"

static const uint32_t sha256_initial_state[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
};

static const uint32_t sha256_round_constants[64] = {
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

static inline void software_sha256_round(uint32_t *a, uint32_t *b,
                                         uint32_t *c, uint32_t *d,
                                         uint32_t *e, uint32_t *f,
                                         uint32_t *g, uint32_t *h,
                                         uint32_t constant, uint32_t word) {
    const uint32_t sum1 = rotate_right(*e, 6u) ^ rotate_right(*e, 11u)
                          ^ rotate_right(*e, 25u);
    const uint32_t choice = *g ^ (*e & (*f ^ *g));
    const uint32_t temp1 = *h + sum1 + choice + constant + word;
    const uint32_t sum0 = rotate_right(*a, 2u) ^ rotate_right(*a, 13u)
                          ^ rotate_right(*a, 22u);
    const uint32_t majority = (*a & *b) | (*c & (*a | *b));
    const uint32_t temp2 = sum0 + majority;
    *h = *g;
    *g = *f;
    *f = *e;
    *e = *d + temp1;
    *d = *c;
    *c = *b;
    *b = *a;
    *a = temp1 + temp2;
}

#ifdef __riscv
#define SOFTWARE_SHA256_ROTATED_ROUND(a, b, c, d, e, f, g, h, constant, word) \
    do {                                                                       \
        const uint32_t rotated_sum1 = rotate_right((e), 6u)                    \
            ^ rotate_right((e), 11u) ^ rotate_right((e), 25u);                 \
        const uint32_t rotated_choice = (g) ^ ((e) & ((f) ^ (g)));             \
        const uint32_t rotated_temp1 = (h) + rotated_sum1 + rotated_choice     \
            + (constant) + (word);                                             \
        const uint32_t rotated_sum0 = rotate_right((a), 2u)                    \
            ^ rotate_right((a), 13u) ^ rotate_right((a), 22u);                 \
        const uint32_t rotated_majority = ((a) & (b))                          \
            | ((c) & ((a) | (b)));                                             \
        (d) += rotated_temp1;                                                   \
        (h) = rotated_temp1 + rotated_sum0 + rotated_majority;                  \
    } while (0)

#define SOFTWARE_SHA256_ROTATED_GROUP4(round, a, b, c, d, e, f, g, h, words) \
    do {                                                                        \
        SOFTWARE_SHA256_ROTATED_ROUND(                                          \
            a, b, c, d, e, f, g, h, sha256_round_constants[(round)],           \
            (words)[(round)]);                                                  \
        SOFTWARE_SHA256_ROTATED_ROUND(                                          \
            h, a, b, c, d, e, f, g, sha256_round_constants[(round) + 1u],      \
            (words)[(round) + 1u]);                                             \
        SOFTWARE_SHA256_ROTATED_ROUND(                                          \
            g, h, a, b, c, d, e, f, sha256_round_constants[(round) + 2u],      \
            (words)[(round) + 2u]);                                             \
        SOFTWARE_SHA256_ROTATED_ROUND(                                          \
            f, g, h, a, b, c, d, e, sha256_round_constants[(round) + 3u],      \
            (words)[(round) + 3u]);                                             \
    } while (0)
#endif

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
        software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                              sha256_round_constants[round], schedule[round]);
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

static __attribute__((optimize("unroll-loops"))) void
__not_in_flash_func(software_sha256_compress_header_tail)(
    uint32_t digest[8], const software_bitcoin_hasher_t *hasher,
    uint32_t nonce_word) {
    uint32_t schedule[64];
    schedule[0] = hasher->tail_words[0];
    schedule[1] = hasher->tail_words[1];
    schedule[2] = hasher->tail_words[2];
    schedule[3] = nonce_word;
    schedule[4] = 0x80000000u;
    for (unsigned word = 5u; word < 15u; ++word) {
        schedule[word] = 0u;
    }
    schedule[15] = 80u * 8u;
    schedule[16] = hasher->tail_schedule16;
    schedule[17] = hasher->tail_schedule17;
    const uint32_t nonce_sigma0 = rotate_right(nonce_word, 7u)
        ^ rotate_right(nonce_word, 18u) ^ (nonce_word >> 3u);
    schedule[18] = hasher->tail_schedule18_base + nonce_sigma0;
    schedule[19] = hasher->tail_schedule19_base + nonce_word;
    for (unsigned word = 20u; word < 31u; ++word) {
        const uint32_t x = schedule[word - 15u];
        const uint32_t y = schedule[word - 2u];
        const uint32_t sigma0 = rotate_right(x, 7u)
                                ^ rotate_right(x, 18u) ^ (x >> 3u);
        const uint32_t sigma1 = rotate_right(y, 17u)
                                ^ rotate_right(y, 19u) ^ (y >> 10u);
        schedule[word] = schedule[word - 16u] + schedule[word - 7u]
                         + sigma0 + sigma1;
    }
    const uint32_t y31 = schedule[29];
    const uint32_t sigma1_31 = rotate_right(y31, 17u)
        ^ rotate_right(y31, 19u) ^ (y31 >> 10u);
    schedule[31] = hasher->tail_schedule31_base + schedule[24] + sigma1_31;
    const uint32_t y32 = schedule[30];
    const uint32_t sigma1_32 = rotate_right(y32, 17u)
        ^ rotate_right(y32, 19u) ^ (y32 >> 10u);
    schedule[32] = hasher->tail_schedule32_base + schedule[25] + sigma1_32;
    for (unsigned word = 33u; word < 64u; ++word) {
        const uint32_t x = schedule[word - 15u];
        const uint32_t y = schedule[word - 2u];
        const uint32_t sigma0 = rotate_right(x, 7u)
                                ^ rotate_right(x, 18u) ^ (x >> 3u);
        const uint32_t sigma1 = rotate_right(y, 17u)
                                ^ rotate_right(y, 19u) ^ (y >> 10u);
        schedule[word] = schedule[word - 16u] + schedule[word - 7u]
                         + sigma0 + sigma1;
    }

    // Only W3 varies in round 3. Complete that round from job-level partials
    // instead of recalculating its rotates and boolean functions per nonce.
    const uint32_t round3_temp1 = hasher->tail_round3_temp1_base + nonce_word;
    uint32_t a = round3_temp1 + hasher->tail_round3_temp2;
    uint32_t b = hasher->tail_round3_state[0];
    uint32_t c = hasher->tail_round3_state[1];
    uint32_t d = hasher->tail_round3_state[2];
    uint32_t e = hasher->tail_round3_state[3] + round3_temp1;
    uint32_t f = hasher->tail_round3_state[4];
    uint32_t g = hasher->tail_round3_state[5];
    uint32_t h = hasher->tail_round3_state[6];

    // On Cortex-M33, spelling out the fixed padding addends lets GCC schedule
    // these rounds faster. The same shape regresses Hazard3, so retain its
    // measured generic loop.
#ifndef __riscv
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[4] + 0x80000000u, 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[5], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[6], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[7], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[8], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[9], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[10], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[11], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[12], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[13], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[14], 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[15] + 80u * 8u, 0u);
#else
    for (unsigned round = 4u; round < 16u; ++round) {
        software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                              sha256_round_constants[round], schedule[round]);
    }
#endif
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          hasher->tail_round16_addend, 0u);
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          hasher->tail_round17_addend, 0u);
    for (unsigned round = 18u; round < 64u; ++round) {
        software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                              sha256_round_constants[round], schedule[round]);
    }

    digest[0] = hasher->midstate[0] + a;
    digest[1] = hasher->midstate[1] + b;
    digest[2] = hasher->midstate[2] + c;
    digest[3] = hasher->midstate[3] + d;
    digest[4] = hasher->midstate[4] + e;
    digest[5] = hasher->midstate[5] + f;
    digest[6] = hasher->midstate[6] + g;
    digest[7] = hasher->midstate[7] + h;
}

static __attribute__((optimize("unroll-loops"))) void
__not_in_flash_func(software_sha256_compress_digest)(
    uint32_t digest[]) {
#ifdef __riscv
    uint32_t schedule[64];
    memcpy(schedule, digest, 8u * sizeof(schedule[0]));
#else
#define schedule digest
#endif
    schedule[8] = 0x80000000u;
    for (unsigned word = 9u; word < 15u; ++word) {
        schedule[word] = 0u;
    }
    schedule[15] = 32u * 8u;
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

    uint32_t a = sha256_initial_state[0];
    uint32_t b = sha256_initial_state[1];
    uint32_t c = sha256_initial_state[2];
    uint32_t d = sha256_initial_state[3];
    uint32_t e = sha256_initial_state[4];
    uint32_t f = sha256_initial_state[5];
    uint32_t g = sha256_initial_state[6];
    uint32_t h = sha256_initial_state[7];

    for (unsigned round = 0u; round < 8u; ++round) {
        software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                              sha256_round_constants[round], schedule[round]);
    }
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[8] + 0x80000000u, 0u);
    for (unsigned round = 9u; round < 15u; ++round) {
        software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                              sha256_round_constants[round], 0u);
    }
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[15] + 32u * 8u, 0u);
    for (unsigned round = 16u; round < 64u; ++round) {
        software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                              sha256_round_constants[round], schedule[round]);
    }

#ifdef __riscv
    digest[0] = sha256_initial_state[0] + a;
    digest[1] = sha256_initial_state[1] + b;
    digest[2] = sha256_initial_state[2] + c;
    digest[3] = sha256_initial_state[3] + d;
    digest[4] = sha256_initial_state[4] + e;
    digest[5] = sha256_initial_state[5] + f;
    digest[6] = sha256_initial_state[6] + g;
    digest[7] = sha256_initial_state[7] + h;
#else
    schedule[0] = sha256_initial_state[0] + a;
    schedule[1] = sha256_initial_state[1] + b;
    schedule[2] = sha256_initial_state[2] + c;
    schedule[3] = sha256_initial_state[3] + d;
    schedule[4] = sha256_initial_state[4] + e;
    schedule[5] = sha256_initial_state[5] + f;
    schedule[6] = sha256_initial_state[6] + g;
    schedule[7] = sha256_initial_state[7] + h;
#undef schedule
#endif
}

// After 61 rounds, e is the value that shifts into h after rounds 61--63.
// Therefore the final digest's numerical word 7 is IV7 + e_61. This permits
// exact rejection for the common Bitcoin target whose most-significant word
// is zero without executing the final three rounds.
static __attribute__((optimize("unroll-loops"))) uint32_t
__not_in_flash_func(software_sha256_digest_high_word_after_round61)(
    uint32_t digest[]) {
#ifdef __riscv
    uint32_t schedule[61];
    memcpy(schedule, digest, 8u * sizeof(schedule[0]));
#else
#define schedule digest
#endif
    // The rejection result consumes rounds 0..60 only. Do not materialize
    // W61..W63: those words belong exclusively to the deliberately skipped
    // final three rounds.
    schedule[8] = 0x80000000u;
    for (unsigned word = 9u; word < 15u; ++word) {
        schedule[word] = 0u;
    }
    schedule[15] = 32u * 8u;
    for (unsigned word = 16u; word < 61u; ++word) {
        const uint32_t x = schedule[word - 15u];
        const uint32_t y = schedule[word - 2u];
        const uint32_t sigma0 = rotate_right(x, 7u)
                                ^ rotate_right(x, 18u) ^ (x >> 3u);
        const uint32_t sigma1 = rotate_right(y, 17u)
                                ^ rotate_right(y, 19u) ^ (y >> 10u);
        schedule[word] = schedule[word - 16u] + schedule[word - 7u]
                         + sigma0 + sigma1;
    }

    uint32_t a = sha256_initial_state[0];
    uint32_t b = sha256_initial_state[1];
    uint32_t c = sha256_initial_state[2];
    uint32_t d = sha256_initial_state[3];
    uint32_t e = sha256_initial_state[4];
    uint32_t f = sha256_initial_state[5];
    uint32_t g = sha256_initial_state[6];
    uint32_t h = sha256_initial_state[7];

    for (unsigned round = 0u; round < 8u; ++round) {
        software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                              sha256_round_constants[round], schedule[round]);
    }
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[8] + 0x80000000u, 0u);
    for (unsigned round = 9u; round < 15u; ++round) {
        software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                              sha256_round_constants[round], 0u);
    }
    software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                          sha256_round_constants[15] + 32u * 8u, 0u);
#ifdef __riscv
    for (unsigned round = 16u; round < 56u; round += 8u) {
        SOFTWARE_SHA256_ROTATED_GROUP4(round, a, b, c, d, e, f, g, h,
                                       schedule);
        SOFTWARE_SHA256_ROTATED_GROUP4(round + 4u, e, f, g, h, a, b, c, d,
                                       schedule);
    }
    SOFTWARE_SHA256_ROTATED_GROUP4(56u, a, b, c, d, e, f, g, h, schedule);
#else
    for (unsigned round = 16u; round < 60u; ++round) {
        software_sha256_round(&a, &b, &c, &d, &e, &f, &g, &h,
                              sha256_round_constants[round], schedule[round]);
    }
#endif
    // Only e after round 60 becomes the final digest's high word. Compute the
    // live half of the terminal round: new e = old d + T1. New a/T2 and the
    // remaining state rotation are dead for this exact rejection decision.
#ifdef __riscv
    // One final four-round group leaves logical a..h in physical e..d.
    const uint32_t terminal_d = h;
    const uint32_t terminal_e = a;
    const uint32_t terminal_f = b;
    const uint32_t terminal_g = c;
    const uint32_t terminal_h = d;
#else
    const uint32_t terminal_d = d;
    const uint32_t terminal_e = e;
    const uint32_t terminal_f = f;
    const uint32_t terminal_g = g;
    const uint32_t terminal_h = h;
#endif
    const uint32_t sum1 = rotate_right(terminal_e, 6u)
                          ^ rotate_right(terminal_e, 11u)
                          ^ rotate_right(terminal_e, 25u);
    const uint32_t choice = terminal_g
                            ^ (terminal_e & (terminal_f ^ terminal_g));
    const uint32_t temp1 = terminal_h + sum1 + choice
                           + sha256_round_constants[60] + schedule[60];
    const uint32_t result = sha256_initial_state[7] + terminal_d + temp1;
#ifndef __riscv
#undef schedule
#endif
    return result;
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
    memcpy(hasher->tail_round3_state, hasher->midstate,
           sizeof(hasher->tail_round3_state));
    uint32_t *round_state = hasher->tail_round3_state;
    for (unsigned round = 0u; round < 3u; ++round) {
        software_sha256_round(&round_state[0], &round_state[1],
                              &round_state[2], &round_state[3],
                              &round_state[4], &round_state[5],
                              &round_state[6], &round_state[7],
                              sha256_round_constants[round],
                              hasher->tail_words[round]);
    }
    const uint32_t round3_a = round_state[0];
    const uint32_t round3_b = round_state[1];
    const uint32_t round3_c = round_state[2];
    const uint32_t round3_e = round_state[4];
    const uint32_t round3_f = round_state[5];
    const uint32_t round3_g = round_state[6];
    const uint32_t round3_h = round_state[7];
    const uint32_t round3_sum1 = rotate_right(round3_e, 6u)
        ^ rotate_right(round3_e, 11u) ^ rotate_right(round3_e, 25u);
    const uint32_t round3_choice = round3_g
        ^ (round3_e & (round3_f ^ round3_g));
    hasher->tail_round3_temp1_base = round3_h + round3_sum1
        + round3_choice + sha256_round_constants[3];
    const uint32_t round3_sum0 = rotate_right(round3_a, 2u)
        ^ rotate_right(round3_a, 13u) ^ rotate_right(round3_a, 22u);
    const uint32_t round3_majority = (round3_a & round3_b)
        | (round3_c & (round3_a | round3_b));
    hasher->tail_round3_temp2 = round3_sum0 + round3_majority;
    const uint32_t x16 = hasher->tail_words[1];
    hasher->tail_schedule16 = hasher->tail_words[0]
        + (rotate_right(x16, 7u) ^ rotate_right(x16, 18u) ^ (x16 >> 3u));
    const uint32_t x17 = hasher->tail_words[2];
    const uint32_t y17 = 80u * 8u;
    hasher->tail_schedule17 = hasher->tail_words[1]
        + (rotate_right(x17, 7u) ^ rotate_right(x17, 18u) ^ (x17 >> 3u))
        + (rotate_right(y17, 17u) ^ rotate_right(y17, 19u) ^ (y17 >> 10u));
    hasher->tail_round16_addend = sha256_round_constants[16]
        + hasher->tail_schedule16;
    hasher->tail_round17_addend = sha256_round_constants[17]
        + hasher->tail_schedule17;
    const uint32_t y18 = hasher->tail_schedule16;
    hasher->tail_schedule18_base = hasher->tail_words[2]
        + (rotate_right(y18, 17u) ^ rotate_right(y18, 19u) ^ (y18 >> 10u));
    const uint32_t x19 = 0x80000000u;
    const uint32_t y19 = hasher->tail_schedule17;
    hasher->tail_schedule19_base =
        (rotate_right(x19, 7u) ^ rotate_right(x19, 18u) ^ (x19 >> 3u))
        + (rotate_right(y19, 17u) ^ rotate_right(y19, 19u) ^ (y19 >> 10u));
    const uint32_t x31 = hasher->tail_schedule16;
    hasher->tail_schedule31_base = 80u * 8u
        + (rotate_right(x31, 7u) ^ rotate_right(x31, 18u) ^ (x31 >> 3u));
    const uint32_t x32 = hasher->tail_schedule17;
    hasher->tail_schedule32_base = hasher->tail_schedule16
        + (rotate_right(x32, 7u) ^ rotate_right(x32, 18u) ^ (x32 >> 3u));
}

void software_bitcoin_hash_nonce(const software_bitcoin_hasher_t *hasher,
                                 uint32_t nonce,
                                 uint8_t hash[32]) {
#ifdef __riscv
    uint32_t second_schedule[8];
#else
    uint32_t second_schedule[64];
#endif
    software_sha256_compress_header_tail(second_schedule, hasher,
                                         __builtin_bswap32(nonce));
    software_sha256_compress_digest(second_schedule);

    for (size_t word = 0u; word < 8u; ++word) {
        const uint32_t encoded = __builtin_bswap32(second_schedule[word]);
        memcpy(&hash[word * 4u], &encoded, sizeof(encoded));
    }
}

uint32_t software_bitcoin_hash_nonce_high_word_be(
    const software_bitcoin_hasher_t *hasher, uint32_t nonce) {
#ifdef __riscv
    uint32_t second_schedule[8];
#else
    uint32_t second_schedule[61];
#endif
    software_sha256_compress_header_tail(second_schedule, hasher,
                                         __builtin_bswap32(nonce));
    return software_sha256_digest_high_word_after_round61(second_schedule);
}

#if MINER_PROFILE
void software_profile_header_tail(const software_bitcoin_hasher_t *hasher,
                                  uint32_t nonce, uint32_t scratch[64]) {
    software_sha256_compress_header_tail(scratch, hasher,
                                         __builtin_bswap32(nonce));
}

void software_profile_digest_full(uint32_t scratch[64]) {
    software_sha256_compress_digest(scratch);
}

uint32_t software_profile_digest_filter(uint32_t scratch[64]) {
    return software_sha256_digest_high_word_after_round61(scratch);
}
#endif
