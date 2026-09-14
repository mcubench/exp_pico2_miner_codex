#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/clocks.h"
#ifdef __riscv
#include "hardware/dma.h"
#endif
#include "hardware/structs/sysinfo.h"
#include "pico/bootrom/lock.h"
#include "pico/multicore.h"
#include "pico/sha256.h"
#include "pico/stdlib.h"

#include "software_sha256.h"

_Static_assert(PICO_RP2350A == 1, "miner target must use the RP2350A package");

#ifdef __riscv
#define CPU_ARCH "RISCV-HAZARD3"
#define BENCHMARK_PATH "persistent-first-block-dma-e06a"
#define MINING_LOOP_OPTIONS __attribute__((optimize("unroll-loops")))
#else
#define CPU_ARCH "ARM-M33"
#define BENCHMARK_PATH "batched-accounting-e04c"
#define MINING_LOOP_OPTIONS __attribute__((optimize("unroll-loops")))
#endif

#define BITCOIN_HEADER_BYTES 80u
#define HASH_BYTES SHA256_RESULT_BYTES
#define NONCE_OFFSET 76u
#define BENCHMARK_MIN_US 2000000ull
#define BENCHMARK_BATCH 1000u
#define MINING_REPORT_INTERVAL 100000u

#ifndef MINER_USE_CORE1
#define MINER_USE_CORE1 1
#endif

#if MINER_SYS_CLOCK_KHZ > 150000
#define CLOCK_PROFILE "experimental-overclock"
#else
#define CLOCK_PROFILE "stock"
#endif

// Bitcoin genesis block header in the serialized byte order hashed by miners.
static const uint8_t genesis_header[BITCOIN_HEADER_BYTES] = {
    0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2,
    0x7a, 0xc7, 0x2c, 0x3e, 0x67, 0x76, 0x8f, 0x61,
    0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32,
    0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a,
    0x29, 0xab, 0x5f, 0x49,
    0xff, 0xff, 0x00, 0x1d,
    0x1d, 0xac, 0x2b, 0x7c,
};

// Raw SHA-256 digest bytes. Bitcoin displays this value in reverse order.
static const uint8_t genesis_hash_raw[HASH_BYTES] = {
    0x6f, 0xe2, 0x8c, 0x0a, 0xb6, 0xf1, 0xb3, 0x72,
    0xc1, 0xa6, 0xa2, 0x46, 0xae, 0x63, 0xf7, 0x4f,
    0x93, 0x1e, 0x83, 0x65, 0xe1, 0x5a, 0x08, 0x9c,
    0x68, 0xd6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t sha256_empty[HASH_BYTES] = {
    0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
    0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
    0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
    0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55,
};

static const uint8_t sha256_abc[HASH_BYTES] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};

#include "oracle_vectors.inc"

typedef struct bitcoin_hasher {
    // Numeric SHA words for the unpadded 80-byte Bitcoin header.
    uint32_t header_words[20];
#ifdef __riscv
    int dma_channel;
#endif
    bool locked;
} bitcoin_hasher_t;

static void print_bitcoin_hash(const uint8_t hash[HASH_BYTES]) {
    for (int i = (int)HASH_BYTES - 1; i >= 0; --i) {
        printf("%02x", hash[i]);
    }
}

// Hash through RP2350's hardware SHA-256 peripheral. DMA is intentionally
// disabled: Bitcoin's 32/80-byte messages are too small to repay setup cost.
static bool hardware_sha256(const uint8_t *data, size_t size, uint8_t hash[HASH_BYTES]) {
    pico_sha256_state_t state;
    sha256_result_t result;
    const int status = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, false);
    if (status != PICO_OK) {
        return false;
    }
    pico_sha256_update_blocking(&state, data, size);
    pico_sha256_finish(&state, &result);
    memcpy(hash, result.bytes, HASH_BYTES);
    return true;
}

static inline __attribute__((always_inline)) void sha256_write_first_block(
    bitcoin_hasher_t *hasher) {
#ifdef __riscv
    // The channel configuration and fixed WDATA destination persist for the
    // complete job. Only the incrementing source and transfer count need to
    // be restored for each invariant first header block.
    dma_channel_set_read_addr((uint)hasher->dma_channel,
                              &hasher->header_words[0], false);
    dma_channel_set_trans_count((uint)hasher->dma_channel, 16u, true);
    dma_channel_wait_for_finish_blocking((uint)hasher->dma_channel);
#else
    // START establishes the ready/reset state, and ordered MMIO stores ensure
    // it reaches the peripheral before these writes. The inter-block feeder
    // still waits explicitly after word 16 starts compression.
    const uint32_t *words = hasher->header_words;
    sha256_put_word(words[0]);
    sha256_put_word(words[1]);
    sha256_put_word(words[2]);
    sha256_put_word(words[3]);
    sha256_put_word(words[4]);
    sha256_put_word(words[5]);
    sha256_put_word(words[6]);
    sha256_put_word(words[7]);
    sha256_put_word(words[8]);
    sha256_put_word(words[9]);
    sha256_put_word(words[10]);
    sha256_put_word(words[11]);
    sha256_put_word(words[12]);
    sha256_put_word(words[13]);
    sha256_put_word(words[14]);
    sha256_put_word(words[15]);
#endif
}

static inline __attribute__((always_inline)) void sha256_write_header_tail(
    const uint32_t words[20]) {
    // Header words 16..19 are followed by SHA-256 padding for an 80-byte
    // message. Emit constants directly instead of loading a padded SRAM block.
    sha256_wait_ready_blocking();
    sha256_put_word(words[16]);
    sha256_put_word(words[17]);
    sha256_put_word(words[18]);
    sha256_put_word(words[19]);
    sha256_put_word(0x80000000u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(BITCOIN_HEADER_BYTES * 8u);
}

static void bitcoin_hasher_begin(bitcoin_hasher_t *hasher,
                                 const uint8_t header[BITCOIN_HEADER_BYTES]) {
    memset(hasher, 0, sizeof(*hasher));
    for (size_t i = 0u; i < BITCOIN_HEADER_BYTES / sizeof(uint32_t); ++i) {
        uint32_t little_endian_word;
        memcpy(&little_endian_word, &header[i * sizeof(uint32_t)],
               sizeof(little_endian_word));
        hasher->header_words[i] = __builtin_bswap32(little_endian_word);
    }

    bootrom_acquire_lock_blocking(BOOTROM_LOCK_SHA_256);
    hasher->locked = true;
#ifdef __riscv
    hasher->dma_channel = dma_claim_unused_channel(true);
    dma_channel_config_t dma_config =
        dma_channel_get_default_config((uint)hasher->dma_channel);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_write_increment(&dma_config, false);
    channel_config_set_dreq(&dma_config, DREQ_SHA256);
    dma_channel_configure((uint)hasher->dma_channel, &dma_config,
                          sha256_get_write_addr(), &hasher->header_words[0],
                          16u, false);
#endif
    sha256_set_bswap(false);
#ifdef __riscv
    sha256_set_dma_size(4u);
#endif
    sha256_err_not_ready_clear();
}

static void bitcoin_hasher_end(bitcoin_hasher_t *hasher) {
#ifdef __riscv
    if (hasher->dma_channel >= 0) {
        dma_channel_cleanup((uint)hasher->dma_channel);
        dma_channel_unclaim((uint)hasher->dma_channel);
        hasher->dma_channel = -1;
    }
#endif
    if (hasher->locked) {
        bootrom_release_lock(BOOTROM_LOCK_SHA_256);
        hasher->locked = false;
    }
}

static inline __attribute__((always_inline)) void bitcoin_hasher_hash_nonce_unchecked(
    bitcoin_hasher_t *hasher,
    uint32_t nonce) {
    // The SHA input is held as numeric big-endian words, while Bitcoin
    // serializes the nonce little-endian at byte offset 76.
    hasher->header_words[NONCE_OFFSET / sizeof(uint32_t)] =
        __builtin_bswap32(nonce);

    sha256_start();
    sha256_write_first_block(hasher);
    sha256_write_header_tail(hasher->header_words);
    sha256_wait_valid_blocking();
    // Preserve the complete first digest before START resets the engine. The
    // explicit locals allow both compilers to keep the handoff in registers.
    const uint32_t digest0 = sha256_hw->sum[0];
    const uint32_t digest1 = sha256_hw->sum[1];
    const uint32_t digest2 = sha256_hw->sum[2];
    const uint32_t digest3 = sha256_hw->sum[3];
    const uint32_t digest4 = sha256_hw->sum[4];
    const uint32_t digest5 = sha256_hw->sum[5];
    const uint32_t digest6 = sha256_hw->sum[6];
    const uint32_t digest7 = sha256_hw->sum[7];

    sha256_start();
    sha256_put_word(digest0);
    sha256_put_word(digest1);
    sha256_put_word(digest2);
    sha256_put_word(digest3);
    sha256_put_word(digest4);
    sha256_put_word(digest5);
    sha256_put_word(digest6);
    sha256_put_word(digest7);
    sha256_put_word(0x80000000u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(0u);
    sha256_put_word(HASH_BYTES * 8u);
    sha256_wait_valid_blocking();
}

static inline __attribute__((always_inline)) bool bitcoin_hasher_hash_nonce(
    bitcoin_hasher_t *hasher,
    uint32_t nonce) {
    bitcoin_hasher_hash_nonce_unchecked(hasher, nonce);
    return !sha256_err_not_ready();
}

static inline __attribute__((always_inline)) void capture_current_hash(
    sha256_result_t *hash) {
    for (size_t i = 0; i < 8u; ++i) {
        hash->words[i] = __builtin_bswap32(sha256_hw->sum[i]);
    }
}

// Expand Bitcoin's nBits representation into a little-endian uint256 target.
static bool compact_to_target_le(uint32_t compact, uint8_t target[HASH_BYTES]) {
    const uint32_t exponent = compact >> 24u;
    uint32_t mantissa = compact & 0x007fffffu;
    memset(target, 0, HASH_BYTES);

    if (exponent <= 3u) {
        mantissa >>= 8u * (3u - exponent);
    }
    const bool negative = mantissa != 0u && (compact & 0x00800000u) != 0u;
    const bool overflow = mantissa != 0u
                          && (exponent > 34u
                              || (mantissa > 0xffu && exponent > 33u)
                              || (mantissa > 0xffffu && exponent > 32u));
    if (negative || overflow || mantissa == 0u) {
        return false;
    }
    if (exponent <= 3u) {
        target[0] = (uint8_t)mantissa;
        target[1] = (uint8_t)(mantissa >> 8u);
        target[2] = (uint8_t)(mantissa >> 16u);
        return true;
    }

    const uint32_t offset = exponent - 3u;
    for (uint32_t byte = 0u; byte < 3u; ++byte) {
        const uint8_t value = (uint8_t)(mantissa >> (8u * byte));
        if (offset + byte < HASH_BYTES) {
            target[offset + byte] = value;
        } else if (value != 0u) {
            return false;
        }
    }
    return true;
}

// The hardware digest byte array is Bitcoin's little-endian uint256 storage.
static bool hash_words_meet_target(const sha256_result_t *hash_le,
                                   const sha256_result_t *target_le) {
    for (int i = 7; i >= 0; --i) {
        if (hash_le->words[i] < target_le->words[i]) {
            return true;
        }
        if (hash_le->words[i] > target_le->words[i]) {
            return false;
        }
    }
    return true;
}

static inline __attribute__((always_inline)) bool current_hash_meets_target(
    const sha256_result_t *target_le) {
    // Byte reversal cannot change whether a word is zero. Bitcoin difficulty
    // targets normally have a zero most-significant word, so reject the common
    // nonzero SUM7 case before entering the ordered uint256 comparison.
    if (target_le->words[7] == 0u && sha256_hw->sum[7] != 0u) {
        return false;
    }
    // Compare the most-significant little-endian word first. Nearly every
    // difficulty-1 candidate is rejected after reading only SUM7.
    for (int i = 7; i >= 0; --i) {
        const uint32_t hash_word = __builtin_bswap32(sha256_hw->sum[i]);
        if (hash_word < target_le->words[i]) {
            return true;
        }
        if (hash_word > target_le->words[i]) {
            return false;
        }
    }
    return true;
}

static uint32_t oracle_xorshift32(uint32_t state) {
    state ^= state << 13u;
    state ^= state >> 17u;
    state ^= state << 5u;
    return state;
}

static bool run_optimized_oracle_vectors(void) {
    uint32_t state = ORACLE_VECTOR_SEED;
    for (uint32_t vector = 0u; vector < ORACLE_VECTOR_COUNT; ++vector) {
        uint8_t header[BITCOIN_HEADER_BYTES];
        for (size_t word = 0u; word < BITCOIN_HEADER_BYTES / 4u; ++word) {
            state = oracle_xorshift32(state);
            header[word * 4u] = (uint8_t)state;
            header[word * 4u + 1u] = (uint8_t)(state >> 8u);
            header[word * 4u + 2u] = (uint8_t)(state >> 16u);
            header[word * 4u + 3u] = (uint8_t)(state >> 24u);
        }
        state = oracle_xorshift32(state);
        const uint32_t nonce = state;

        bitcoin_hasher_t hasher;
        software_bitcoin_hasher_t software_hasher;
        sha256_result_t hash;
        uint8_t software_hash[HASH_BYTES];
        bitcoin_hasher_begin(&hasher, header);
        const bool hashed = bitcoin_hasher_hash_nonce(&hasher, nonce);
        capture_current_hash(&hash);
        bitcoin_hasher_end(&hasher);
        software_bitcoin_hasher_begin(&software_hasher, header);
        software_bitcoin_hash_nonce(&software_hasher, nonce, software_hash);
        if (!hashed || memcmp(hash.bytes, oracle_expected[vector], HASH_BYTES) != 0
            || memcmp(software_hash, oracle_expected[vector], HASH_BYTES) != 0) {
            printf("TEST:FAIL kat=optimized_oracle vector=%" PRIu32
                   " nonce=%" PRIu32 "\n",
                   vector, nonce);
            return false;
        }
    }
    printf("TEST:PASS kat=optimized_oracle engines=hardware,software-midstate"
           " cases=%u fixture_sha256=%s\n",
           ORACLE_VECTOR_COUNT, ORACLE_FIXTURE_SHA256);
    return true;
}

static bool run_target_tests(void) {
    sha256_result_t hash = {0};
    sha256_result_t target = {0};
    uint8_t decoded[HASH_BYTES];
    bool passed = true;

    target.words[7] = 1u;
    passed &= hash_words_meet_target(&hash, &target);
    hash.words[7] = 1u;
    passed &= hash_words_meet_target(&hash, &target);
    hash.words[7] = 2u;
    passed &= !hash_words_meet_target(&hash, &target);
    hash.words[7] = 1u;
    hash.words[0] = 2u;
    target.words[0] = 1u;
    passed &= !hash_words_meet_target(&hash, &target);

    passed &= compact_to_target_le(0x1d00ffffu, decoded);
    passed &= !compact_to_target_le(0x1d80ffffu, decoded);
    passed &= !compact_to_target_le(0x01003456u, decoded);
    passed &= !compact_to_target_le(0x23000001u, decoded);
    passed &= compact_to_target_le(0x220000ffu, decoded)
              && decoded[31] == 0xffu;
    passed &= compact_to_target_le(0x2100ffffu, decoded)
              && decoded[30] == 0xffu && decoded[31] == 0xffu;

    printf("TEST:%s kat=target_boundaries cases=10\n",
           passed ? "PASS" : "FAIL");
    return passed;
}

static bool check_vector(const char *name,
                         const uint8_t *message,
                         size_t size,
                         const uint8_t expected[HASH_BYTES]) {
    uint8_t actual[HASH_BYTES];
    const bool passed = hardware_sha256(message, size, actual)
                        && memcmp(actual, expected, HASH_BYTES) == 0;
    printf("TEST:%s kat=%s engine=RP2350-SHA256\n", passed ? "PASS" : "FAIL", name);
    return passed;
}

static bool run_sha_error_sticky_test(void) {
    // Prove the premise used by batched error checking: an illegal WDATA write
    // latches ERR_WDATA_NOT_RDY, START does not erase it, and the documented
    // SDK clear operation removes it. Match the Pico SDK hardware test's
    // non-DMA stimulus: a long unpaced burst guarantees that writes overlap a
    // compression regardless of CPU/peripheral timing.
    bootrom_acquire_lock_blocking(BOOTROM_LOCK_SHA_256);
    sha256_set_bswap(false);
    sha256_err_not_ready_clear();
    sha256_start();
    for (uint32_t word = 0u; word < 2500u; ++word) {
        sha256_put_word(word);
    }
    sha256_wait_ready_blocking();
    const bool latched = sha256_err_not_ready();
    sha256_start();
    const bool survived_start = sha256_err_not_ready();
    sha256_err_not_ready_clear();
    const bool cleared = !sha256_err_not_ready();
    bootrom_release_lock(BOOTROM_LOCK_SHA_256);

    const bool passed = latched && survived_start && cleared;
    printf("TEST:%s kat=sha_error_sticky cases=3 burst_words=2500"
           " latched=%u survived_start=%u cleared=%u\n",
           passed ? "PASS" : "FAIL", latched, survived_start, cleared);
    return passed;
}

static bool run_known_answer_tests(void) {
    bool passed = true;
    static const uint8_t abc[] = {'a', 'b', 'c'};
    sha256_result_t hash = {0};
    sha256_result_t target;
    bitcoin_hasher_t hasher;

    passed &= check_vector("nist_empty", NULL, 0u, sha256_empty);
    passed &= check_vector("nist_abc", abc, sizeof(abc), sha256_abc);
    passed &= run_sha_error_sticky_test();
    passed &= run_optimized_oracle_vectors();
    passed &= run_target_tests();

    bitcoin_hasher_begin(&hasher, genesis_header);
    const bool genesis_hashed = bitcoin_hasher_hash_nonce(&hasher, 2083236893u);
    capture_current_hash(&hash);
    const bool genesis_passed = genesis_hashed
                                && memcmp(hash.bytes, genesis_hash_raw, HASH_BYTES) == 0;
    printf("TEST:%s kat=bitcoin_genesis hash=", genesis_passed ? "PASS" : "FAIL");
    print_bitcoin_hash(hash.bytes);
    printf("\n");
    passed &= genesis_passed;

    const bool target_valid = compact_to_target_le(0x1d00ffffu, target.bytes);
    const uint32_t first_nonce = 2083236800u;
    const uint32_t expected_nonce = 2083236893u;
    uint32_t found_nonce = 0u;
    uint32_t attempts = 0u;
    bool found = false;
    if (target_valid) {
        for (uint32_t nonce = first_nonce; nonce <= expected_nonce; ++nonce) {
            ++attempts;
            if (!bitcoin_hasher_hash_nonce(&hasher, nonce)) {
                break;
            }
            if (current_hash_meets_target(&target)) {
                found_nonce = nonce;
                found = true;
                capture_current_hash(&hash);
                break;
            }
        }
    }
    const bool mining_passed = found && found_nonce == expected_nonce
                               && memcmp(hash.bytes, genesis_hash_raw, HASH_BYTES) == 0;
    printf("TEST:%s kat=bitcoin_nonce_search nonce=%" PRIu32
           " attempts=%" PRIu32 " hash=",
           mining_passed ? "PASS" : "FAIL", found_nonce, attempts);
    print_bitcoin_hash(hash.bytes);
    printf("\n");
    passed &= mining_passed;
    bitcoin_hasher_end(&hasher);
    return passed;
}

static bool run_benchmark(void) {
    bitcoin_hasher_t hasher;
    uint32_t nonce = 0u;
    uint64_t hashes = 0u;
    volatile uint8_t checksum = 0u;
    bitcoin_hasher_begin(&hasher, genesis_header);

    const uint64_t started_us = time_us_64();
    uint64_t elapsed_us;
    do {
        for (uint32_t i = 0; i < BENCHMARK_BATCH; ++i) {
#ifdef __riscv
            bitcoin_hasher_hash_nonce_unchecked(&hasher, nonce++);
#else
            if (!bitcoin_hasher_hash_nonce(&hasher, nonce++)) {
                printf("FAULT type=sha256_hardware benchmark=1\n");
                bitcoin_hasher_end(&hasher);
                return false;
            }
#endif
            checksum ^= (uint8_t)(sha256_hw->sum[0] >> 24u);
        }
#ifdef __riscv
        if (sha256_err_not_ready()) {
            printf("FAULT type=sha256_hardware benchmark=1"
                   " invalid_batch=%u\n", BENCHMARK_BATCH);
            bitcoin_hasher_end(&hasher);
            return false;
        }
#endif
        hashes += BENCHMARK_BATCH;
        elapsed_us = time_us_64() - started_us;
    } while (elapsed_us < BENCHMARK_MIN_US);
    bitcoin_hasher_end(&hasher);

    const uint64_t rate = (hashes * 1000000ull + elapsed_us / 2u) / elapsed_us;
    printf("BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256"
           " path=" BENCHMARK_PATH
           " arch=%s clock_hz=%" PRIu32 " hashes=%" PRIu64
           " elapsed_us=%" PRIu64 " hash_rate_hs=%" PRIu64
           " checksum=%02x temperature=disabled\n",
           CPU_ARCH, clock_get_hz(clk_sys), hashes, elapsed_us, rate, checksum);
    return true;
}

static void run_software_benchmark(void) {
    software_bitcoin_hasher_t hasher;
    uint8_t hash[HASH_BYTES];
    uint32_t nonce = 0u;
    uint64_t hashes = 0u;
    volatile uint8_t checksum = 0u;
    software_bitcoin_hasher_begin(&hasher, genesis_header);

    const uint64_t started_us = time_us_64();
    uint64_t elapsed_us;
    do {
        for (uint32_t i = 0u; i < BENCHMARK_BATCH; ++i) {
            software_bitcoin_hash_nonce(&hasher, nonce++, hash);
            checksum ^= hash[0];
        }
        hashes += BENCHMARK_BATCH;
        elapsed_us = time_us_64() - started_us;
    } while (elapsed_us < BENCHMARK_MIN_US);

    const uint64_t rate = (hashes * 1000000ull + elapsed_us / 2u) / elapsed_us;
    printf("SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256"
           " path=portable-midstate-e09a arch=%s clock_hz=%" PRIu32
           " hashes=%" PRIu64 " elapsed_us=%" PRIu64
           " hash_rate_hs=%" PRIu64 " checksum=%02x temperature=disabled\n",
           CPU_ARCH, clock_get_hz(clk_sys), hashes, elapsed_us, rate, checksum);
}

#if !MINER_USE_CORE1
static MINING_LOOP_OPTIONS void mine_forever(uint led_pin) {
    sha256_result_t hash;
    sha256_result_t target;
    bitcoin_hasher_t hasher;
    uint32_t nonce = 0u;
    uint32_t since_report = 0u;
    uint64_t total_hashes = 0u;
    uint64_t report_started_us = time_us_64();
    bool led_on = false;

    if (!compact_to_target_le(0x1d00ffffu, target.bytes)) {
        printf("FAULT type=invalid_compact_target bits=1d00ffff\n");
        return;
    }
    bitcoin_hasher_begin(&hasher, genesis_header);

    printf("MINING:START header=bitcoin-genesis target_bits=1d00ffff start_nonce=0"
           " note=standalone-stale-work\n");
    while (true) {
#ifdef __riscv
        bitcoin_hasher_hash_nonce_unchecked(&hasher, nonce);
#else
        // On M33, report-boundary batching slightly reduced sustained rate;
        // retain its faster checked-per-hash mining layout.
        if (!bitcoin_hasher_hash_nonce(&hasher, nonce)) {
            printf("FAULT type=sha256_hardware nonce=%" PRIu32 "\n", nonce);
            bitcoin_hasher_end(&hasher);
            return;
        }
#endif
        ++since_report;
        const bool candidate = current_hash_meets_target(&target);

        // ERR_WDATA_NOT_RDY is sticky (proved at startup). A candidate is
        // always validated immediately, before it can be published.
        if (candidate) {
#ifdef __riscv
            if (sha256_err_not_ready()) {
                printf("FAULT type=sha256_hardware nonce=%" PRIu32
                       " invalid_batch=%" PRIu32 "\n",
                       nonce, since_report);
                bitcoin_hasher_end(&hasher);
                return;
            }
#endif
            capture_current_hash(&hash);
            printf("SHARE:FOUND nonce=%" PRIu32 " hash=", nonce);
            print_bitcoin_hash(hash.bytes);
            printf(" total_hashes=%" PRIu64 "\n",
                   total_hashes + since_report);
        }
        ++nonce;

        if (since_report == MINING_REPORT_INTERVAL) {
#ifdef __riscv
            // Reuse the existing report boundary rather than adding a hot-path
            // counter and comparison. On error, discard the whole unvalidated
            // interval before accounting or reporting it as useful work.
            if (sha256_err_not_ready()) {
                printf("FAULT type=sha256_hardware next_nonce=%" PRIu32
                       " invalid_batch=%u\n",
                       nonce, MINING_REPORT_INTERVAL);
                bitcoin_hasher_end(&hasher);
                return;
            }
#endif
            const uint64_t now_us = time_us_64();
            const uint64_t elapsed_us = now_us - report_started_us;
            const uint64_t rate = ((uint64_t)since_report * 1000000ull
                                   + elapsed_us / 2u) / elapsed_us;
            total_hashes += since_report;
            printf("MINING:PROGRESS arch=%s nonce=%" PRIu32
                   " total_hashes=%" PRIu64 " hash_rate_hs=%" PRIu64
                   " temperature=disabled\n",
                   CPU_ARCH, nonce, total_hashes, rate);
            since_report = 0u;
            report_started_us = now_us;
            led_on = !led_on;
            gpio_put(led_pin, led_on);
        }
    }
}
#else
enum mining_message {
    MINING_MESSAGE_PROGRESS = 0x50524752u,
    MINING_MESSAGE_SHARE = 0x53485245u,
    MINING_MESSAGE_FAULT = 0x4641554cu,
};

static void mining_fifo_push_u64(uint64_t value) {
    multicore_fifo_push_blocking((uint32_t)value);
    multicore_fifo_push_blocking((uint32_t)(value >> 32u));
}

static uint64_t mining_fifo_pop_u64(void) {
    const uint64_t low = multicore_fifo_pop_blocking();
    return low | ((uint64_t)multicore_fifo_pop_blocking() << 32u);
}

static void mining_worker_fault(uint32_t code, uint32_t nonce,
                                uint32_t invalid_batch) {
    multicore_fifo_push_blocking(MINING_MESSAGE_FAULT);
    multicore_fifo_push_blocking(code);
    multicore_fifo_push_blocking(nonce);
    multicore_fifo_push_blocking(invalid_batch);
    while (true) {
        tight_loop_contents();
    }
}

static MINING_LOOP_OPTIONS void mining_worker_core1(void) {
    sha256_result_t hash;
    sha256_result_t target;
    bitcoin_hasher_t hasher;
    uint32_t nonce = 0u;
    uint32_t since_report = 0u;
    uint64_t total_hashes = 0u;
    uint64_t report_started_us = time_us_64();

    if (!compact_to_target_le(0x1d00ffffu, target.bytes)) {
        mining_worker_fault(1u, nonce, 0u);
    }
    bitcoin_hasher_begin(&hasher, genesis_header);

    while (true) {
#ifdef __riscv
        bitcoin_hasher_hash_nonce_unchecked(&hasher, nonce);
#else
        if (!bitcoin_hasher_hash_nonce(&hasher, nonce)) {
            bitcoin_hasher_end(&hasher);
            mining_worker_fault(2u, nonce, 1u);
        }
#endif
        ++since_report;
        const bool candidate = current_hash_meets_target(&target);

        if (candidate) {
#ifdef __riscv
            if (sha256_err_not_ready()) {
                bitcoin_hasher_end(&hasher);
                mining_worker_fault(2u, nonce, since_report);
            }
#endif
            capture_current_hash(&hash);
            multicore_fifo_push_blocking(MINING_MESSAGE_SHARE);
            multicore_fifo_push_blocking(nonce);
            mining_fifo_push_u64(total_hashes + since_report);
            for (size_t word = 0u; word < 8u; ++word) {
                multicore_fifo_push_blocking(hash.words[word]);
            }
        }
        ++nonce;

        if (since_report == MINING_REPORT_INTERVAL) {
#ifdef __riscv
            if (sha256_err_not_ready()) {
                bitcoin_hasher_end(&hasher);
                mining_worker_fault(2u, nonce, MINING_REPORT_INTERVAL);
            }
#endif
            const uint64_t now_us = time_us_64();
            const uint64_t elapsed_us = now_us - report_started_us;
            const uint64_t rate = ((uint64_t)since_report * 1000000ull
                                   + elapsed_us / 2u) / elapsed_us;
            total_hashes += since_report;
            multicore_fifo_push_blocking(MINING_MESSAGE_PROGRESS);
            multicore_fifo_push_blocking(nonce);
            mining_fifo_push_u64(total_hashes);
            mining_fifo_push_u64(rate);
            since_report = 0u;
            report_started_us = now_us;
        }
    }
}

static void mine_forever(uint led_pin) {
    bool led_on = false;
    multicore_fifo_drain();
    printf("MINING:START header=bitcoin-genesis target_bits=1d00ffff start_nonce=0"
           " worker_core=1 note=standalone-stale-work\n");
    multicore_launch_core1(mining_worker_core1);

    while (true) {
        const uint32_t message = multicore_fifo_pop_blocking();
        if (message == MINING_MESSAGE_PROGRESS) {
            const uint32_t nonce = multicore_fifo_pop_blocking();
            const uint64_t total_hashes = mining_fifo_pop_u64();
            const uint64_t rate = mining_fifo_pop_u64();
            printf("MINING:PROGRESS arch=%s worker_core=1 nonce=%" PRIu32
                   " total_hashes=%" PRIu64 " hash_rate_hs=%" PRIu64
                   " temperature=disabled\n",
                   CPU_ARCH, nonce, total_hashes, rate);
            led_on = !led_on;
            gpio_put(led_pin, led_on);
        } else if (message == MINING_MESSAGE_SHARE) {
            sha256_result_t hash;
            const uint32_t nonce = multicore_fifo_pop_blocking();
            const uint64_t total_hashes = mining_fifo_pop_u64();
            for (size_t word = 0u; word < 8u; ++word) {
                hash.words[word] = multicore_fifo_pop_blocking();
            }
            printf("SHARE:FOUND nonce=%" PRIu32 " hash=", nonce);
            print_bitcoin_hash(hash.bytes);
            printf(" total_hashes=%" PRIu64 "\n", total_hashes);
        } else if (message == MINING_MESSAGE_FAULT) {
            const uint32_t code = multicore_fifo_pop_blocking();
            const uint32_t nonce = multicore_fifo_pop_blocking();
            const uint32_t invalid_batch = multicore_fifo_pop_blocking();
            printf("FAULT type=%s worker_core=1 nonce=%" PRIu32
                   " invalid_batch=%" PRIu32 "\n",
                   code == 1u ? "invalid_compact_target" : "sha256_hardware",
                   nonce, invalid_batch);
            while (true) {
                gpio_xor_mask64(1ull << led_pin);
                sleep_ms(100u);
            }
        } else {
            printf("FAULT type=multicore_protocol message=%08" PRIx32 "\n",
                   message);
            while (true) {
                gpio_xor_mask64(1ull << led_pin);
                sleep_ms(100u);
            }
        }
    }
}
#endif

int main(void) {
    const bool clock_configured = set_sys_clock_khz(MINER_SYS_CLOCK_KHZ, false);
    stdio_init_all();

    const uint led_pin = PICO_DEFAULT_LED_PIN;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin, true);

    // Give the host time to enumerate USB CDC and attach the monitor. A fixed
    // delay also keeps headless operation independent of host DTR behaviour.
    sleep_ms(3500u);
    if (!clock_configured) {
        printf("FAULT type=system_clock requested_khz=%u\n", MINER_SYS_CLOCK_KHZ);
        while (true) {
            gpio_xor_mask(1u << led_pin);
            sleep_ms(100u);
        }
    }
    const uint32_t chip_id = sysinfo_hw->chip_id;
    const uint32_t package_sel = sysinfo_hw->package_sel;
    printf("BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A"
           " arch=%s engine=RP2350-SHA256 temperature=disabled"
           " clock_profile=%s requested_clock_khz=%u actual_clock_hz=%" PRIu32
           " sysinfo_package_sel=%" PRIu32 " chip_id=%08" PRIx32
           " silicon_revision=%u\n",
           CPU_ARCH, CLOCK_PROFILE, (unsigned)MINER_SYS_CLOCK_KHZ,
           clock_get_hz(clk_sys), package_sel, chip_id, rp2350_chip_version());
    if (package_sel != 1u) {
        printf("FAULT type=package_mismatch expected_sysinfo_package_sel=1"
               " actual_sysinfo_package_sel=%" PRIu32 "\n",
               package_sel);
        while (true) {
            gpio_xor_mask(1u << led_pin);
            sleep_ms(100u);
        }
    }
    if (!run_known_answer_tests()) {
        printf("TEST:SUMMARY pass=0 fail=1\n");
        while (true) {
            gpio_xor_mask(1u << led_pin);
            sleep_ms(100u);
        }
    }
    printf("TEST:SUMMARY pass=7 fail=0\n");

    if (!run_benchmark()) {
        while (true) {
            gpio_xor_mask64(1ull << led_pin);
            sleep_ms(100u);
        }
    }
    run_software_benchmark();
    mine_forever(led_pin);
    return 0;
}
