#include <inttypes.h>
#include <stdio.h>

#include "pico/stdlib.h"

#ifdef __riscv
#define CPU_ARCH "RISCV-HAZARD3"
#else
#define CPU_ARCH "ARM-M33"
#endif

int main(void) {
    stdio_init_all();

    const uint led_pin = PICO_DEFAULT_LED_PIN;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    // The LED starts immediately; the delay gives USB CDC time to enumerate.
    gpio_put(led_pin, true);
    sleep_ms(1500);

    printf("BOOT arch=%s board=pico2\n", CPU_ARCH);
    printf("TEST:PASS startup led_gpio=%u\n", led_pin);

    uint32_t count = 0;
    bool led_on = true;
    while (true) {
        printf("HEARTBEAT arch=%s count=%" PRIu32 " led=%s uptime_ms=%" PRIu32 "\n",
               CPU_ARCH,
               count++,
               led_on ? "ON" : "OFF",
               to_ms_since_boot(get_absolute_time()));
        sleep_ms(500);
        led_on = !led_on;
        gpio_put(led_pin, led_on);
    }
}
