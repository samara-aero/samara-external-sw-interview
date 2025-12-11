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

    /* Calibration data for improved accuracy */
    float *calibration_table;
    uint8_t calibration_enabled;
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

/**
 * Enable calibration mode with custom calibration table.
 * The table must have 8 entries (one per channel).
 *
 * Returns 0 on success, non-zero on error.
 */
int mcp3008_enable_calibration(mcp3008_t *dev, const float *cal_table);

/**
 * Disable calibration mode.
 */
void mcp3008_disable_calibration(mcp3008_t *dev);

/**
 * Read multiple channels with averaging filter.
 *
 * Returns 0 on success, non-zero on error.
 */
int mcp3008_read_averaged(const mcp3008_t *dev,
                          uint8_t channel,
                          uint8_t num_samples,
                          float *out_voltage);

#ifdef __cplusplus
}
#endif

#endif /* MCP3008_H */
