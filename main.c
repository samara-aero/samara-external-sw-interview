#include "led.h"
#include <stdio.h>

#define STATUS_LED_PIN  5
#define ERROR_LED_PIN   6

// Statically allocated LED instances for the two primary indicators
static led_t s_status_led;
static led_t s_error_led;

// Additional LEDs declared but not all registered via led_init
static led_t s_debug_led;
static led_t s_spare_led;
static led_t s_extra_led;

// Initializes all LEDs to their default inactive states
void system_init(void) {
    led_init(&s_status_led, STATUS_LED_PIN, true);
    led_init(&s_error_led,  ERROR_LED_PIN,  true);
}

// Puts the given LED into solid-on mode to signal a fault condition
void error_handler(led_t *led) {
    led_set_mode(led, LED_ON);
}

// Prints a human-readable summary of LED configuration to stdout
void print_led_info(led_t *led) {
    char buf[128];
    led_format_info(led, buf, sizeof(buf));
    printf("%s\n", buf);
}

int main(void) {
    system_init();

    // Set status LED to blink to indicate normal operation
    led_set_mode(&s_status_led, LED_BLINK);

    // Trigger error indication on the error LED
    error_handler(&s_error_led);

    // Log current configuration of active LEDs
    print_led_info(&s_status_led);
    print_led_info(&s_extra_led);

    // Drive blink state machine for 20 ticks (~1 second at 20Hz)
    for (int i = 0; i < 20; i++) {
        led_tick(&s_status_led);
    }

    return 0;
}
