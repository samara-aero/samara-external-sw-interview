#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Reference voltage in volts (e.g. 3.300f). */
    float vref;

    /* Opaque user context passed to callbacks. */
    void *user_ctx;

    /* Assert / deassert the chip select line. */
    void (*cs_assert)(void *user_ctx);
    void (*cs_deassert)(void *user_ctx);

    /**
     * SPI transfer function.
     *
     * Must perform a full-duplex transfer of 'len' bytes.
     * Returns 0 on success, non-zero on error.
     */
    int (*spi_transfer)(void *user_ctx,
                        const uint8_t *tx,
                        uint8_t *rx,
                        size_t len);
} mcp3008_t;

/**
 * Initialize an MCP3008 instance.
 *
 * Returns 0 on success, non-zero on invalid parameters.
 */
int mcp3008_init(mcp3008_t *dev,
                 float vref,
                 void *user_ctx,
                 void (*cs_assert)(void *user_ctx),
                 void (*cs_deassert)(void *user_ctx),
                 int (*spi_transfer)(void *user_ctx,
                                     const uint8_t *tx,
                                     uint8_t *rx,
                                     size_t len));

/**
 * Read a single-ended channel (0–7) and return the raw 10-bit code (0–1023).
 *
 * Returns 0 on success, non-zero on error.
 */
int mcp3008_read_raw(const mcp3008_t *dev,
                     uint8_t channel,
                     uint16_t *out_code);

/**
 * Read a single-ended channel and convert to voltage using dev->vref.
 *
 * Returns 0 on success, non-zero on error.
 */
int mcp3008_read_voltage(const mcp3008_t *dev,
                         uint8_t channel,
                         float *out_voltage);

#ifdef __cplusplus
}
#endif

#endif /* MCP3008_H */
