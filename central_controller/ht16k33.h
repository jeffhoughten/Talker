#pragma once

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ---------------------------------------------------------------------------
// HT16K33 1.2" 4-digit 7-segment backpack driver
// Communicates over I2C.  Caller must select the correct TCA9548A channel
// before calling any function here.
// ---------------------------------------------------------------------------

// I2C address — Adafruit backpack default (A0-A2 unset = 0x70).
// NOTE: this matches TCA_ADDR (also 0x70).  If both are on the same raw bus
// at the same time you'd have a collision.  The TCA channel select isolates
// them: when a TCA channel is selected that channel's SDA/SCL are connected
// to the bus, all others are disconnected, so 0x70 is unambiguous per channel.
#define HT16K33_ADDR  0x70

// Digit positions (0 = leftmost, 4 = rightmost; position 2 = colon)
#define HT16K33_POS_D0   0
#define HT16K33_POS_D1   1
#define HT16K33_POS_COLON 2
#define HT16K33_POS_D2   3
#define HT16K33_POS_D3   4

// ---------------------------------------------------------------------------
// Segment bit encoding (standard common-cathode mapping used by Adafruit)
//  bit 0 = a (top)
//  bit 1 = b (upper right)
//  bit 2 = c (lower right)
//  bit 3 = d (bottom)
//  bit 4 = e (lower left)
//  bit 5 = f (upper left)
//  bit 6 = g (middle)
//  bit 7 = decimal point
// ---------------------------------------------------------------------------
#define SEG_BLANK 0x00
#define SEG_DASH  0x40   // only g (middle bar)
#define SEG_COLON_ON  0x02  // bit 1 in colon position word

// ---------------------------------------------------------------------------
// Context — one per physical display
// ---------------------------------------------------------------------------
typedef struct {
    i2c_inst_t *i2c;
    uint8_t     addr;
    uint8_t     buf[5];  // display positions 0-4 (position 2 = colon word)
} HT16K33;

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// Initialise: turn on oscillator and display, full brightness.
void ht16k33_init(HT16K33 *d, i2c_inst_t *i2c, uint8_t addr);

// Set a single digit position (0–1 and 3–4) to a decimal value 0–9.
// dot=true lights the decimal point on that digit.
void ht16k33_write_digit(HT16K33 *d, uint8_t pos, uint8_t digit, bool dot);

// Set a position to raw segment bitmask.
void ht16k33_write_raw(HT16K33 *d, uint8_t pos, uint8_t segments);

// Enable or disable the colon between positions 1 and 3.
void ht16k33_set_colon(HT16K33 *d, bool on);

// Push buf to the display over I2C.
void ht16k33_flush(const HT16K33 *d);

// Clear all digits and colon, then flush.
void ht16k33_clear(HT16K33 *d);

// Convenience: print a 4-digit unsigned value left-padded with blanks.
void ht16k33_print_number(HT16K33 *d, uint16_t value, bool colon, bool leading_zeros);

// Display MM:SS from milliseconds (≥ 60 000 ms) or SS.cs below 60 seconds.
// Mirrors the original IOManager::updateTimer() logic.
void ht16k33_show_time(HT16K33 *d, uint32_t ms_remaining);

// Show a short string (up to 4 chars) using segment approximations.
// Useful for "dOnE", "----", etc.
void ht16k33_print_str(HT16K33 *d, const char *s);

// Set display brightness 0 (dimmest) – 15 (brightest).
void ht16k33_set_brightness(HT16K33 *d, uint8_t level);
