#include "mcp3008.h"

#define MCP3008_MAX_CHANNEL   7u
#define MCP3008_MAX_CODE      1023u

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

    return 0;
}
