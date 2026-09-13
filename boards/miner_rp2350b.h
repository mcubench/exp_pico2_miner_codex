/*
 * Repository-local board definition for the attached RP2350B miner board.
 *
 * Only properties already exercised by this project are specified here:
 * 4 MiB W25Q080-compatible flash operation and the LED on GPIO 25. Do not add
 * carrier-board peripherals until their wiring has been verified.
 */

#ifndef _BOARDS_MINER_RP2350B_H
#define _BOARDS_MINER_RP2350B_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)

#define MINER_RP2350B_BOARD
#define PICO_RP2350A 0

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (4 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#endif

pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

#endif
