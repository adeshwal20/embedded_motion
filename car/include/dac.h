#ifndef CAR_DAC_H
#define CAR_DAC_H

#include <stdbool.h>
#include <stdint.h>

/**
 * MCP4921 @ SPI0: CS=GPIO21, SCK=GPIO22, SDI/MOSI=GPIO23 (/LDAC tied GND on PCB).
 * Drives buzzer driver (NPN); use dac_set_level() 0..4095 for loudness / on-off.
 */
void dac_init(void);

/** Raw 12-bit DAC code (0 = off, 4095 ≈ full scale). */
void dac_set_level(uint16_t level_12);

/** Convenience: buzzer roughly on (mid-high code) or off. */
void dac_buzzer_set(bool on);

#endif
