#include "dac.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

/* MCP4921 — matches schematic: CS 21, SCK 22, MOSI 23; SPI mode 0. */
#ifndef DAC_SPI_INST
#define DAC_SPI_INST spi0
#endif
#ifndef DAC_PIN_CS
#define DAC_PIN_CS 21u
#endif
#ifndef DAC_PIN_SCK
#define DAC_PIN_SCK 22u
#endif
#ifndef DAC_PIN_MOSI
#define DAC_PIN_MOSI 23u
#endif

#ifndef DAC_SPI_HZ
#define DAC_SPI_HZ 4000000u
#endif

/*
 * 16-bit MCP4921 write: [15:12] config (0x3 = write DAC A, unbuffered, 1× gain, active)
 * [11:0] data. Vout = (code/4096)*Vref when GA=1× with Vref=3.3V.
 */
static void mcp4921_write12(uint16_t value12) {
    uint16_t cmd = (uint16_t)(0x3000u | (value12 & 0x0FFFu));
    uint8_t buf[2] = {(uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFFu)};

    gpio_put(DAC_PIN_CS, 0);
    spi_write_blocking(DAC_SPI_INST, buf, 2);
    gpio_put(DAC_PIN_CS, 1);
}

void dac_init(void) {
    spi_init(DAC_SPI_INST, DAC_SPI_HZ);
    spi_set_format(DAC_SPI_INST, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(DAC_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(DAC_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(DAC_PIN_CS);
    gpio_set_dir(DAC_PIN_CS, GPIO_OUT);
    gpio_put(DAC_PIN_CS, 1);

    mcp4921_write12(0u);
}

void dac_set_level(uint16_t level_12) {
    mcp4921_write12(level_12);
}

void dac_buzzer_set(bool on) {
    /* ~50% Vout when “on” — adjust if buzzer is too loud/quiet. */
    mcp4921_write12(on ? 2048u : 0u);
}
