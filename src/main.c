#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/clocks.h"
#include "pico/bootrom/lock.h"
#include "pico/sha256.h"
#include "pico/stdlib.h"

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

typedef struct bitcoin_hasher {
    // An 80-byte Bitcoin header occupies two padded SHA-256 blocks.
    uint32_t header_blocks[32];
    // The intermediate 32-byte digest occupies one padded SHA-256 block.
    uint32_t second_block[16];
    bool locked;
} bitcoin_hasher_t;

static void write_le32(uint8_t *destination, uint32_t value) {
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8u);
    destination[2] = (uint8_t)(value >> 16u);
    destination[3] = (uint8_t)(value >> 24u);
}

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

static void sha256_process_words(const uint32_t *words,
                                 size_t word_count,
                                 sha256_result_t *result) {
    sha256_err_not_ready_clear();
    sha256_start();
    for (size_t i = 0; i < word_count; ++i) {
        sha256_wait_ready_blocking();
        sha256_put_word(words[i]);
    }
    sha256_wait_valid_blocking();
    sha256_get_result(result, SHA256_BIG_ENDIAN);
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
}

static void bitcoin_hasher_end(bitcoin_hasher_t *hasher) {
    if (hasher->locked) {
        bootrom_release_lock(BOOTROM_LOCK_SHA_256);
        hasher->locked = false;
    }
}

static bool bitcoin_hasher_hash_nonce(bitcoin_hasher_t *hasher,
                                      uint32_t nonce,
                                      uint8_t hash[HASH_BYTES]) {
    sha256_result_t first_hash;
    sha256_result_t final_hash;
    uint8_t *header_bytes = (uint8_t *)hasher->header_blocks;
    uint8_t *second_bytes = (uint8_t *)hasher->second_block;

    write_le32(&header_bytes[NONCE_OFFSET], nonce);
    sha256_process_words(hasher->header_blocks, 32u, &first_hash);
    if (sha256_err_not_ready()) {
        return false;
    }
    memcpy(second_bytes, first_hash.bytes, HASH_BYTES);
    sha256_process_words(hasher->second_block, 16u, &final_hash);
    if (sha256_err_not_ready()) {
        return false;
    }
    memcpy(hash, final_hash.bytes, HASH_BYTES);
    return true;
}

// Expand Bitcoin's nBits representation into a little-endian uint256 target.
static bool compact_to_target_le(uint32_t compact, uint8_t target[HASH_BYTES]) {
    const uint32_t exponent = compact >> 24u;
    uint32_t mantissa = compact & 0x007fffffu;
    memset(target, 0, HASH_BYTES);

    if ((compact & 0x00800000u) != 0u || mantissa == 0u || exponent > 32u) {
        return false;
    }
    if (exponent <= 3u) {
        mantissa >>= 8u * (3u - exponent);
        target[0] = (uint8_t)mantissa;
        target[1] = (uint8_t)(mantissa >> 8u);
        target[2] = (uint8_t)(mantissa >> 16u);
        return true;
    }

    const uint32_t offset = exponent - 3u;
    if (offset + 3u > HASH_BYTES) {
        return false;
    }
    target[offset] = (uint8_t)mantissa;
    target[offset + 1u] = (uint8_t)(mantissa >> 8u);
    target[offset + 2u] = (uint8_t)(mantissa >> 16u);
    return true;
}

// The hardware digest byte array is Bitcoin's little-endian uint256 storage.
static bool hash_meets_target(const uint8_t hash[HASH_BYTES],
                              const uint8_t target_le[HASH_BYTES]) {
    for (int i = (int)HASH_BYTES - 1; i >= 0; --i) {
        if (hash[i] < target_le[i]) {
            return true;
        }
        if (hash[i] > target_le[i]) {
            return false;
        }
    }
    return true;
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
    uint8_t hash[HASH_BYTES] = {0};
    uint8_t target[HASH_BYTES];
    bitcoin_hasher_t hasher;

    passed &= check_vector("nist_empty", NULL, 0u, sha256_empty);
    passed &= check_vector("nist_abc", abc, sizeof(abc), sha256_abc);

    bitcoin_hasher_begin(&hasher, genesis_header);
    const bool genesis_passed = bitcoin_hasher_hash_nonce(&hasher, 2083236893u, hash)
                                && memcmp(hash, genesis_hash_raw, HASH_BYTES) == 0;
    printf("TEST:%s kat=bitcoin_genesis hash=", genesis_passed ? "PASS" : "FAIL");
    print_bitcoin_hash(hash);
    printf("\n");
    passed &= genesis_passed;

    const bool target_valid = compact_to_target_le(0x1d00ffffu, target);
    const uint32_t first_nonce = 2083236800u;
    const uint32_t expected_nonce = 2083236893u;
    uint32_t found_nonce = 0u;
    uint32_t attempts = 0u;
    bool found = false;
    if (target_valid) {
        for (uint32_t nonce = first_nonce; nonce <= expected_nonce; ++nonce) {
            ++attempts;
            if (!bitcoin_hasher_hash_nonce(&hasher, nonce, hash)) {
                break;
            }
            if (hash_meets_target(hash, target)) {
                found_nonce = nonce;
                found = true;
                break;
            }
        }
    }
    const bool mining_passed = found && found_nonce == expected_nonce
                               && memcmp(hash, genesis_hash_raw, HASH_BYTES) == 0;
    printf("TEST:%s kat=bitcoin_nonce_search nonce=%" PRIu32
           " attempts=%" PRIu32 " hash=",
           mining_passed ? "PASS" : "FAIL", found_nonce, attempts);
    print_bitcoin_hash(hash);
    printf("\n");
    passed &= mining_passed;
    bitcoin_hasher_end(&hasher);
    return passed;
}

static void run_benchmark(void) {
    uint8_t hash[HASH_BYTES];
    bitcoin_hasher_t hasher;
    uint32_t nonce = 0u;
    uint64_t hashes = 0u;
    volatile uint8_t checksum = 0u;
    bitcoin_hasher_begin(&hasher, genesis_header);

    const uint64_t started_us = time_us_64();
    uint64_t elapsed_us;
    do {
        for (uint32_t i = 0; i < BENCHMARK_BATCH; ++i) {
            if (!bitcoin_hasher_hash_nonce(&hasher, nonce++, hash)) {
                printf("FAULT type=sha256_hardware benchmark=1\n");
                bitcoin_hasher_end(&hasher);
                return;
            }
            checksum ^= hash[0];
            ++hashes;
        }
        elapsed_us = time_us_64() - started_us;
    } while (elapsed_us < BENCHMARK_MIN_US);
    bitcoin_hasher_end(&hasher);

    const uint64_t rate = (hashes * 1000000ull + elapsed_us / 2u) / elapsed_us;
    printf("BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256"
           " path=direct-padded-blocks"
           " arch=%s clock_hz=%" PRIu32 " hashes=%" PRIu64
           " elapsed_us=%" PRIu64 " hash_rate_hs=%" PRIu64
           " checksum=%02x\n",
           CPU_ARCH, clock_get_hz(clk_sys), hashes, elapsed_us, rate, checksum);
}

static void mine_forever(uint led_pin) {
    uint8_t hash[HASH_BYTES];
    uint8_t target[HASH_BYTES];
    bitcoin_hasher_t hasher;
    uint32_t nonce = 0u;
    uint32_t since_report = 0u;
    uint64_t total_hashes = 0u;
    uint64_t report_started_us = time_us_64();
    bool led_on = false;

    if (!compact_to_target_le(0x1d00ffffu, target)) {
        printf("FAULT type=invalid_compact_target bits=1d00ffff\n");
        return;
    }
    bitcoin_hasher_begin(&hasher, genesis_header);

    printf("MINING:START header=bitcoin-genesis target_bits=1d00ffff start_nonce=0"
           " note=standalone-stale-work\n");
    while (true) {
        if (!bitcoin_hasher_hash_nonce(&hasher, nonce, hash)) {
            printf("FAULT type=sha256_hardware nonce=%" PRIu32 "\n", nonce);
            bitcoin_hasher_end(&hasher);
            return;
        }
        ++total_hashes;
        ++since_report;

        if (hash_meets_target(hash, target)) {
            printf("SHARE:FOUND nonce=%" PRIu32 " hash=", nonce);
            print_bitcoin_hash(hash);
            printf(" total_hashes=%" PRIu64 "\n", total_hashes);
        }
        ++nonce;

        if (since_report == MINING_REPORT_INTERVAL) {
            const uint64_t now_us = time_us_64();
            const uint64_t elapsed_us = now_us - report_started_us;
            const uint64_t rate = ((uint64_t)since_report * 1000000ull
                                   + elapsed_us / 2u) / elapsed_us;
            printf("MINING:PROGRESS arch=%s nonce=%" PRIu32
                   " total_hashes=%" PRIu64 " hash_rate_hs=%" PRIu64 "\n",
                   CPU_ARCH, nonce, total_hashes, rate);
            since_report = 0u;
            report_started_us = now_us;
            led_on = !led_on;
            gpio_put(led_pin, led_on);
        }
    }
}

int main(void) {
    stdio_init_all();

    const uint led_pin = PICO_DEFAULT_LED_PIN;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin, true);

    // Give the host time to enumerate USB CDC and attach the monitor. A fixed
    // delay also keeps headless operation independent of host DTR behaviour.
    sleep_ms(3500u);
    printf("BOOT app=pico2_bitcoin_miner arch=%s engine=RP2350-SHA256\n", CPU_ARCH);

    if (!run_known_answer_tests()) {
        printf("TEST:SUMMARY pass=0 fail=1\n");
        while (true) {
            gpio_xor_mask(1u << led_pin);
            sleep_ms(100u);
        }
    }
    printf("TEST:SUMMARY pass=4 fail=0\n");

    run_benchmark();
    mine_forever(led_pin);
    return 0;
}
