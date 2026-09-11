#include "ht16k33.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Segment encoding table for 0–9 and A–Z (7-seg approximations)
// ---------------------------------------------------------------------------
static const uint8_t DIGITS[10] = {
    0x3F, // 0: a b c d e f
    0x06, // 1: b c
    0x5B, // 2: a b d e g
    0x4F, // 3: a b c d g
    0x66, // 4: b c f g
    0x6D, // 5: a c d f g
    0x7D, // 6: a c d e f g
    0x07, // 7: a b c
    0x7F, // 8: a b c d e f g
    0x6F, // 9: a b c d f g
};

// Letters (A–Z) — 7-seg approximations
static const uint8_t LETTERS[26] = {
    0x77, // A: a b c e f g
    0x7C, // b: c d e f g
    0x39, // C: a d e f
    0x5E, // d: b c d e g
    0x79, // E: a d e f g
    0x71, // F: a e f g
    0x3D, // G: a c d e f
    0x76, // H: b c e f g
    0x06, // I: b c  (same as 1)
    0x1E, // J: b c d e
    0x76, // K: approximate as H
    0x38, // L: d e f
    0x37, // M: approximate (a b c e f) — imperfect on 7-seg
    0x54, // n: c e g
    0x5C, // o: c d e g
    0x73, // P: a b e f g
    0x67, // q: a b c f g
    0x50, // r: e g
    0x6D, // S: same as 5
    0x78, // t: d e f g
    0x3E, // U: b c d e f
    0x3E, // V: same as U (imperfect)
    0x3E, // W: same as U (imperfect)
    0x76, // X: approximate as H
    0x6E, // Y: b c d f g
    0x5B, // Z: same as 2
};

static uint8_t char_to_seg(char c) {
    if (c >= '0' && c <= '9') return DIGITS[c - '0'];
    if (c >= 'A' && c <= 'Z') return LETTERS[c - 'A'];
    if (c >= 'a' && c <= 'z') return LETTERS[c - 'a'];
    if (c == '-')              return SEG_DASH;
    return SEG_BLANK;
}

// ---------------------------------------------------------------------------
// Low-level I2C writes
// ---------------------------------------------------------------------------
static void cmd(const HT16K33 *d, uint8_t byte) {
    i2c_write_blocking(d->i2c, d->addr, &byte, 1, false);
}

// Write the full 10-byte display RAM (5 positions × 2 bytes; high byte always 0)
static void flush_raw(const HT16K33 *d) {
    uint8_t frame[11];
    frame[0] = 0x00; // RAM start address
    for (int i = 0; i < 5; i++) {
        frame[1 + i * 2] = d->buf[i];
        frame[2 + i * 2] = 0x00;
    }
    i2c_write_blocking(d->i2c, d->addr, frame, 11, false);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void ht16k33_init(HT16K33 *d, i2c_inst_t *i2c, uint8_t addr) {
    d->i2c  = i2c;
    d->addr = addr;
    memset(d->buf, 0, sizeof(d->buf));
    cmd(d, 0x21);       // System setup: oscillator on
    cmd(d, 0xEF);       // Brightness: max (0xE0 | 0x0F)
    cmd(d, 0x81);       // Display on, no blink (0x80 | 0x01)
    flush_raw(d);
}

void ht16k33_write_digit(HT16K33 *d, uint8_t pos, uint8_t digit, bool dot) {
    if (pos > 4 || pos == HT16K33_POS_COLON) return;
    d->buf[pos] = DIGITS[digit % 10] | (dot ? 0x80 : 0x00);
}

void ht16k33_write_raw(HT16K33 *d, uint8_t pos, uint8_t segments) {
    if (pos > 4) return;
    d->buf[pos] = segments;
}

void ht16k33_set_colon(HT16K33 *d, bool on) {
    d->buf[HT16K33_POS_COLON] = on ? SEG_COLON_ON : 0x00;
}

void ht16k33_flush(const HT16K33 *d) {
    flush_raw(d);
}

void ht16k33_clear(HT16K33 *d) {
    memset(d->buf, 0, sizeof(d->buf));
    flush_raw(d);
}

void ht16k33_set_brightness(HT16K33 *d, uint8_t level) {
    if (level > 15) level = 15;
    cmd(d, 0xE0 | level);
}

void ht16k33_print_number(HT16K33 *d, uint16_t value, bool colon, bool leading_zeros) {
    uint8_t digits[4];
    digits[3] = value % 10; value /= 10;
    digits[2] = value % 10; value /= 10;
    digits[1] = value % 10; value /= 10;
    digits[0] = value % 10;

    // Map display positions: pos0, pos1, [colon at pos2], pos3, pos4
    const uint8_t map[4] = { HT16K33_POS_D0, HT16K33_POS_D1,
                              HT16K33_POS_D2,  HT16K33_POS_D3 };
    bool seen_nonzero = false;
    for (int i = 0; i < 4; i++) {
        if (!leading_zeros && !seen_nonzero && digits[i] == 0 && i < 3) {
            d->buf[map[i]] = SEG_BLANK;
        } else {
            d->buf[map[i]] = DIGITS[digits[i]];
            seen_nonzero = true;
        }
    }
    ht16k33_set_colon(d, colon);
    flush_raw(d);
}

// Mirrors IOManager::updateTimer() from the original Arduino code.
void ht16k33_show_time(HT16K33 *d, uint32_t ms) {
    if (ms >= 60000) {
        // Display MM:SS with blinking colon
        uint32_t total_sec = ms / 1000;
        uint8_t  mins = (uint8_t)(total_sec / 60);
        uint8_t  secs = (uint8_t)(total_sec % 60);
        d->buf[HT16K33_POS_D0]    = (mins >= 10) ? DIGITS[mins / 10] : SEG_BLANK;
        d->buf[HT16K33_POS_D1]    = DIGITS[mins % 10];
        d->buf[HT16K33_POS_COLON] = (secs % 2 == 0) ? SEG_COLON_ON : 0x00;
        d->buf[HT16K33_POS_D2]    = DIGITS[secs / 10];
        d->buf[HT16K33_POS_D3]    = DIGITS[secs % 10];
    } else {
        // Display SS.cs (seconds + centiseconds) — decimal point on second digit
        uint32_t cs = ms / 10;  // total centiseconds (0–5999)
        uint8_t  s_tens = (uint8_t)((cs / 100) / 10 % 10);
        uint8_t  s_ones = (uint8_t)((cs / 100) % 10);
        uint8_t  c_tens = (uint8_t)((cs / 10)  % 10);
        uint8_t  c_ones = (uint8_t)(cs % 10);
        d->buf[HT16K33_POS_D0]    = (cs >= 1000) ? DIGITS[s_tens] : SEG_BLANK;
        d->buf[HT16K33_POS_D1]    = DIGITS[s_ones] | 0x80; // decimal point = "SS."
        d->buf[HT16K33_POS_COLON] = 0x00;
        d->buf[HT16K33_POS_D2]    = DIGITS[c_tens];
        d->buf[HT16K33_POS_D3]    = DIGITS[c_ones];
    }
    flush_raw(d);
}

void ht16k33_print_str(HT16K33 *d, const char *s) {
    const uint8_t map[4] = { HT16K33_POS_D0, HT16K33_POS_D1,
                              HT16K33_POS_D2,  HT16K33_POS_D3 };
    memset(d->buf, 0, sizeof(d->buf));
    for (int i = 0; i < 4 && s[i]; i++) {
        d->buf[map[i]] = char_to_seg(s[i]);
    }
    flush_raw(d);
}
