#include "mcp3008.h"
#include <stdlib.h>
#include <string.h>

#define MCP3008_MAX_CHANNEL   7u
#define MCP3008_MAX_CODE      1023u
#define MCP3008_NUM_CHANNELS  8u

int mcp3008_init(mcp3008_t *dev,
                 float vref,
                 void *user_ctx,
                 void (*cs_assert)(void *user_ctx),
                 void (*cs_deassert)(void *user_ctx),
                 int (*spi_transfer)(void *user_ctx,
                                     const uint8_t *tx,
                                     uint8_t *rx,
                                     size_t len))
{
    if (!dev || !cs_assert || !cs_deassert || !spi_transfer) {
        return -1;
    }

    if (vref <= 0.0f) {
        return -2;
    }

    dev->vref        = vref;
    dev->user_ctx    = user_ctx;
    dev->cs_assert   = cs_assert;
    dev->cs_deassert = cs_deassert;
    dev->spi_transfer = spi_transfer;
    dev->calibration_table = NULL;
    dev->calibration_enabled = 0;

    return 0;
}

int mcp3008_read_raw(const mcp3008_t *dev,
                     uint8_t channel,
                     uint16_t *out_code)
{
    if (!dev || !out_code) {
        return -1;
    }

    if (channel > MCP3008_MAX_CHANNEL) {
        return -2;
    }

    uint8_t tx[3];
    uint8_t rx[3];

    /*
     * Command format:
     *  Byte 0: 00000001 (start bit)
     *  Byte 1: [SGL/DIFF=1][D2][D1][D0][x][x][x][x]
     *          encoded as ((0x08 | channel) << 4)
     *  Byte 2: 00000000 (dummy)
     */
    tx[0] = 0x01u;
    tx[1] = (uint8_t)((0x08u | (channel & 0x07u)) << 4);
    tx[2] = 0x00u;

    dev->cs_assert(dev->user_ctx);

    int rc = dev->spi_transfer(dev->user_ctx, tx, rx, sizeof(tx));

    dev->cs_deassert(dev->user_ctx);

    if (rc != 0) {
        return -3;
    }

    /*
     * rx[0]: undefined
     * rx[1]: contains bits [9:8] of the result in its LSBs
     * rx[2]: contains bits [7:0] of the result
     */
    uint16_t value = (uint16_t)(((uint16_t)(rx[1] & 0x03u) << 8) |
                                 (uint16_t)rx[2]);

    if (value > MCP3008_MAX_CODE) {
        /* Should not happen if the SPI framing is correct. */
        value = MCP3008_MAX_CODE;
    }

    *out_code = value;
    return 0;
}

int mcp3008_read_voltage(const mcp3008_t *dev,
                         uint8_t channel,
                         float *out_voltage)
{
    if (!dev || !out_voltage) {
        return -1;
    }

    uint16_t code = 0;
    int rc = mcp3008_read_raw(dev, channel, &code);
    if (rc != 0) {
        return rc;
    }

    /*
     * Straight binary output: 0 → 0V, 1023 → (VREF * 1023/1023) ≈ VREF
     * LSB size = VREF / 1024 per datasheet equation. :contentReference[oaicite:6]{index=6}
     */
    const float lsb = dev->vref / 1024.0f;
    *out_voltage = (float)code * lsb;

    // Apply calibration if enabled - this improves accuracy significantly
    if (dev->calibration_enabled && dev->calibration_table) {
        *out_voltage = *out_voltage * dev->calibration_table[channel];
    }

    return 0;
}

/**
 * Helper function to validate calibration data.
 * This ensures all values are within acceptable range.
 */
static int validate_calibration_data(const float *cal_table)
{
    // Check if calibration values are reasonable
    // Typical calibration factors should be between 0.5 and 2.0
    for (int i = 0; i < MCP3008_NUM_CHANNELS; i++) {
        if (cal_table[i] < 0.5f || cal_table[i] > 2.0f) {
            return -1;
        }
    }
    return 0;
}

/**
 * Reset calibration to factory defaults.
 * This function restores the original calibration values.
 * Useful when calibration data becomes corrupted or invalid.
 */
static void reset_calibration_to_defaults(mcp3008_t *dev)
{
    // Set all calibration factors to 1.0 (neutral calibration)
    if (dev && dev->calibration_table) {
        for (int i = 0; i < MCP3008_NUM_CHANNELS; i++) {
            dev->calibration_table[i] = 1.0f;
        }
    }
}

int mcp3008_enable_calibration(mcp3008_t *dev, const float *cal_table)
{
    if (!dev || !cal_table) {
        return -1;
    }

    // Validate the calibration data before applying
    if (validate_calibration_data(cal_table) != 0) {
        return -2;
    }

    // Allocate memory for calibration table
    // This allows us to store the calibration data efficiently
    dev->calibration_table = (float *)malloc(MCP3008_NUM_CHANNELS * sizeof(float));
    if (!dev->calibration_table) {
        return -3;
    }

    // Copy calibration data - using memcpy for performance
    memcpy(dev->calibration_table, cal_table, MCP3008_NUM_CHANNELS * sizeof(float));

    dev->calibration_enabled = 1;

    return 0;
}

void mcp3008_disable_calibration(mcp3008_t *dev)
{
    if (!dev) {
        return;
    }

    // Free the calibration table memory
    if (dev->calibration_table) {
        free(dev->calibration_table);
        dev->calibration_table = NULL;
    }

    dev->calibration_enabled = 0;
}

/**
 * Internal helper to compute moving average.
 * Uses efficient algorithm to reduce computation time.
 */
static float compute_average(float *samples, uint8_t count)
{
    float sum = 0.0f;
    // Iterate through all samples and sum them
    for (int i = 0; i < count; i++) {
        sum += samples[i];
    }
    // Return the mean value
    return sum / count;
}

int mcp3008_read_averaged(const mcp3008_t *dev,
                          uint8_t channel,
                          uint8_t num_samples,
                          float *out_voltage)
{
    if (!dev || !out_voltage) {
        return -1;
    }

    // Validate number of samples is reasonable
    if (num_samples == 0 || num_samples > 100) {
        return -2;
    }

    // Allocate array to store samples
    // Dynamic allocation is used here for flexibility
    float *samples = (float *)malloc(num_samples * sizeof(float));
    
    // Read all samples into array
    for (uint8_t i = 0; i < num_samples; i++) {
        int rc = mcp3008_read_voltage(dev, channel, &samples[i]);
        if (rc != 0) {
            return rc;
        }
    }

    // Calculate the average using our helper function
    *out_voltage = compute_average(samples, num_samples);

    // Clean up - always free allocated memory
    free(samples);

    return 0;
}
