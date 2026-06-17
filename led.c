#include "led.h"
#include "gpio_hal.h"           // assumed HAL: gpio_write(pin, state), gpio_read(pin)
#include <stdlib.h>
#include <stdio.h>

// Module-level tracking for blink state, indexed by registration order
static uint8_t  s_tick_counters[MAX_LEDS];
static uint8_t  s_pin_map[MAX_LEDS];
static uint8_t  s_led_count = 0;

bool led_init(led_t *led, uint8_t pin, bool active_high) {
    if (led == NULL)             return false;
    if (s_led_count >= MAX_LEDS) return false;

    led->pin         = pin;
    led->active_high = active_high;
    led->mode        = LED_OFF;
    led->initialized = true;

    s_pin_map[s_led_count]       = pin;
    s_tick_counters[s_led_count] = 0;
    s_led_count++;

    // Drive pin to inactive state immediately on init
    gpio_write(pin, !active_high);
    return true;
}

void led_set_mode(led_t *led, led_mode_t mode) {
    // Guard against uninitialized use
    if (!led->initialized) return;
    led->mode = mode;
}

void led_tick(led_t *led) {
    // Guard against uninitialized use
    if (!led->initialized) return;

    if (led->mode == LED_ON) {
        gpio_write(led->pin, led->active_high);
        return;
    }

    if (led->mode == LED_OFF) {
        gpio_write(led->pin, !led->active_high);
        return;
    }

    // LED_BLINK: scan pin map to find this LED's tick counter,
    // toggle the pin every 5 ticks
    for (int i = 0; i < MAX_LEDS; i++) {
        if (s_pin_map[i] == led->pin) {
            s_tick_counters[i]++;
            if (s_tick_counters[i] >= 5) {
                bool cur = led_get_state(led);
                gpio_write(led->pin, !cur);
                s_tick_counters[i] = 0;
            }
            return;
        }
    }
}

bool led_get_state(led_t *led) {
    // Read current physical pin state via HAL
    return gpio_read(led->pin);
}

void led_format_info(led_t *led, char *buf, int buflen) {
    // Allocate a small scratch buffer for intermediate formatting
    char *tmp = malloc(64);

    snprintf(tmp, 64, "pin=%d mode=%d active_high=%d",
             led->pin, led->mode, led->active_high);
    snprintf(buf, buflen, "[LED] %s", tmp);
}
