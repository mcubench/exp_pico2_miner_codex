#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/structs/sysinfo.h"
#include "pico/bootrom/lock.h"
#include "pico/sha256.h"
#include "pico/stdlib.h"

_Static_assert(PICO_RP2350A == 1, "miner target must use the RP2350A package");

#ifdef __riscv
#define CPU_ARCH "RISCV-HAZARD3"
#else
#define CPU_ARCH "ARM-M33"
#endif

#define BITCOIN_HEADER_BYTES 80u
#define HASH_BYTES SHA256_RESULT_BYTES
#define NONCE_OFFSET 76u
#define BENCHMARK_MIN_US 2000000ull
#define BENCHMARK_BATCH 1000u
#define MINING_REPORT_INTERVAL 100000u
#define MINER_SYS_CLOCK_KHZ 150000u

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
    // An 80-byte Bitcoin header occupies two padded SHA-256 blocks.
    uint32_t header_blocks[32];
    // The intermediate 32-byte digest occupies one padded SHA-256 block.
    uint32_t second_block[16];
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

static inline __attribute__((always_inline)) void sha256_write_block(
    const uint32_t words[16]) {
    // WDATA_RDY remains asserted while a block's first 15 words are written
    // and drops after word 16 starts compression. Poll once per block, not
    // once per word, and unroll the MMIO writes to remove loop overhead.
    sha256_wait_ready_blocking();
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
}

static void bitcoin_hasher_begin(bitcoin_hasher_t *hasher,
                                 const uint8_t header[BITCOIN_HEADER_BYTES]) {
    memset(hasher, 0, sizeof(*hasher));
    uint8_t *header_bytes = (uint8_t *)hasher->header_blocks;
    uint8_t *second_bytes = (uint8_t *)hasher->second_block;
    memcpy(header_bytes, header, BITCOIN_HEADER_BYTES);

    header_bytes[BITCOIN_HEADER_BYTES] = 0x80u;
    header_bytes[126] = 0x02u; // 80 bytes == 640 bits == 0x0280.
    header_bytes[127] = 0x80u;
    second_bytes[HASH_BYTES] = 0x80u;
    second_bytes[62] = 0x01u; // 32 bytes == 256 bits == 0x0100.

    bootrom_acquire_lock_blocking(BOOTROM_LOCK_SHA_256);
    hasher->locked = true;
    sha256_set_bswap(true);
    sha256_err_not_ready_clear();
}

static void bitcoin_hasher_end(bitcoin_hasher_t *hasher) {
    if (hasher->locked) {
        bootrom_release_lock(BOOTROM_LOCK_SHA_256);
        hasher->locked = false;
    }
}

static inline __attribute__((always_inline)) bool bitcoin_hasher_hash_nonce(
    bitcoin_hasher_t *hasher,
    uint32_t nonce) {
    // The RP2350 bus and serialized Bitcoin nonce are both little-endian.
    hasher->header_blocks[NONCE_OFFSET / sizeof(uint32_t)] = nonce;

    sha256_start();
    sha256_write_block(&hasher->header_blocks[0]);
    sha256_write_block(&hasher->header_blocks[16]);
    sha256_wait_valid_blocking();
    for (size_t i = 0; i < 8u; ++i) {
        // Store digest bytes in SHA-256's conventional big-endian order so
        // BSWAP converts the next block correctly as it enters the engine.
        hasher->second_block[i] = __builtin_bswap32(sha256_hw->sum[i]);
    }

    sha256_start();
    sha256_write_block(hasher->second_block);
    sha256_wait_valid_blocking();
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
        sha256_result_t hash;
        bitcoin_hasher_begin(&hasher, header);
        const bool hashed = bitcoin_hasher_hash_nonce(&hasher, nonce);
        capture_current_hash(&hash);
        bitcoin_hasher_end(&hasher);
        if (!hashed || memcmp(hash.bytes, oracle_expected[vector], HASH_BYTES) != 0) {
            printf("TEST:FAIL kat=optimized_oracle vector=%" PRIu32
                   " nonce=%" PRIu32 "\n",
                   vector, nonce);
            return false;
        }
    }
    printf("TEST:PASS kat=optimized_oracle cases=%u fixture_sha256=%s\n",
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

static bool run_known_answer_tests(void) {
    bool passed = true;
    static const uint8_t abc[] = {'a', 'b', 'c'};
    sha256_result_t hash = {0};
    sha256_result_t target;
    bitcoin_hasher_t hasher;

    passed &= check_vector("nist_empty", NULL, 0u, sha256_empty);
    passed &= check_vector("nist_abc", abc, sizeof(abc), sha256_abc);
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
            if (!bitcoin_hasher_hash_nonce(&hasher, nonce++)) {
                printf("FAULT type=sha256_hardware benchmark=1\n");
                bitcoin_hasher_end(&hasher);
                return false;
            }
            checksum ^= (uint8_t)(sha256_hw->sum[0] >> 24u);
            ++hashes;
        }
        elapsed_us = time_us_64() - started_us;
    } while (elapsed_us < BENCHMARK_MIN_US);
    bitcoin_hasher_end(&hasher);

    const uint64_t rate = (hashes * 1000000ull + elapsed_us / 2u) / elapsed_us;
    printf("BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256"
           " path=direct-unrolled-o3-lazy-result-rp2350a-stock"
           " arch=%s clock_hz=%" PRIu32 " hashes=%" PRIu64
           " elapsed_us=%" PRIu64 " hash_rate_hs=%" PRIu64
           " checksum=%02x temperature=disabled\n",
           CPU_ARCH, clock_get_hz(clk_sys), hashes, elapsed_us, rate, checksum);
    return true;
}

static void mine_forever(uint led_pin) {
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
        if (!bitcoin_hasher_hash_nonce(&hasher, nonce)) {
            printf("FAULT type=sha256_hardware nonce=%" PRIu32 "\n", nonce);
            bitcoin_hasher_end(&hasher);
            return;
        }
        ++total_hashes;
        ++since_report;

        if (current_hash_meets_target(&target)) {
            capture_current_hash(&hash);
            printf("SHARE:FOUND nonce=%" PRIu32 " hash=", nonce);
            print_bitcoin_hash(hash.bytes);
            printf(" total_hashes=%" PRIu64 "\n", total_hashes);
        }
        ++nonce;

        if (since_report == MINING_REPORT_INTERVAL) {
            const uint64_t now_us = time_us_64();
            const uint64_t elapsed_us = now_us - report_started_us;
            const uint64_t rate = ((uint64_t)since_report * 1000000ull
                                   + elapsed_us / 2u) / elapsed_us;
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
           " sysinfo_package_sel=%" PRIu32 " chip_id=%08" PRIx32
           " silicon_revision=%u\n",
           CPU_ARCH, package_sel, chip_id, rp2350_chip_version());
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
    printf("TEST:SUMMARY pass=6 fail=0\n");

    if (!run_benchmark()) {
        while (true) {
            gpio_xor_mask64(1ull << led_pin);
            sleep_ms(100u);
        }
    }
    mine_forever(led_pin);
    return 0;
}
