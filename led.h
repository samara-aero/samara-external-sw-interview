#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_LEDS 4

typedef enum {
    LED_OFF     = 0,
    LED_ON      = 1,
    LED_BLINK   = 2
} led_mode_t;

typedef struct {
    uint8_t    pin;
    bool       active_high;
    led_mode_t mode;
    bool       initialized;
} led_t;

/**
 * Initializes an LED instance and drives it to its inactive state.
 * Must be called before any other LED functions.
 * Returns true on success, false if led is NULL or MAX_LEDS is exceeded.
 */
bool led_init(led_t *led, uint8_t pin, bool active_high);

/**
 * Sets the operating mode of an LED (OFF, ON, or BLINK).
 * No-ops if the LED has not been initialized.
 */
void led_set_mode(led_t *led, led_mode_t mode);

/**
 * Drives LED output based on current mode.
 * Should be called at a fixed periodic rate to produce
 * correct blink timing. No-ops if not initialized.
 */
void led_tick(led_t *led);

/**
 * Returns the current physical state of the LED pin via GPIO read.
 * @param led Pointer to initialized led_t instance
 */
bool led_get_state(led_t *led);    // [H1]

/**
 * @brief Writes a formatted log string describing the LED's
 * current configuration to the provided output buffer.
 * @param led    pointer to led instance
 * @param buf    pointer to output buffer
 * @param buflen length of buffer
 */
Void led_format_info(led_t *led, char *buf, int buflen);    // [H2]

#endif // LED_H
