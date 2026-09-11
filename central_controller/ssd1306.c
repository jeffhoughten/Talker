#include "ssd1306.h"
#include <string.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// 5×7 font — 96 printable ASCII characters starting at 0x20 (space)
// Each character is 5 columns; each byte is a column of 7 pixels (LSB = top).
// ---------------------------------------------------------------------------
static const uint8_t FONT5X7[][5] = {
    { 0x00,0x00,0x00,0x00,0x00 }, // 0x20 space
    { 0x00,0x00,0x5F,0x00,0x00 }, // !
    { 0x00,0x07,0x00,0x07,0x00 }, // "
    { 0x14,0x7F,0x14,0x7F,0x14 }, // #
    { 0x24,0x2A,0x7F,0x2A,0x12 }, // $
    { 0x23,0x13,0x08,0x64,0x62 }, // %
    { 0x36,0x49,0x55,0x22,0x50 }, // &
    { 0x00,0x05,0x03,0x00,0x00 }, // '
    { 0x00,0x1C,0x22,0x41,0x00 }, // (
    { 0x00,0x41,0x22,0x1C,0x00 }, // )
    { 0x08,0x2A,0x1C,0x2A,0x08 }, // *
    { 0x08,0x08,0x3E,0x08,0x08 }, // +
    { 0x00,0x50,0x30,0x00,0x00 }, // ,
    { 0x08,0x08,0x08,0x08,0x08 }, // -
    { 0x00,0x60,0x60,0x00,0x00 }, // .
    { 0x20,0x10,0x08,0x04,0x02 }, // /
    { 0x3E,0x51,0x49,0x45,0x3E }, // 0
    { 0x00,0x42,0x7F,0x40,0x00 }, // 1
    { 0x42,0x61,0x51,0x49,0x46 }, // 2
    { 0x21,0x41,0x45,0x4B,0x31 }, // 3
    { 0x18,0x14,0x12,0x7F,0x10 }, // 4
    { 0x27,0x45,0x45,0x45,0x39 }, // 5
    { 0x3C,0x4A,0x49,0x49,0x30 }, // 6
    { 0x01,0x71,0x09,0x05,0x03 }, // 7
    { 0x36,0x49,0x49,0x49,0x36 }, // 8
    { 0x06,0x49,0x49,0x29,0x1E }, // 9
    { 0x00,0x36,0x36,0x00,0x00 }, // :
    { 0x00,0x56,0x36,0x00,0x00 }, // ;
    { 0x00,0x08,0x14,0x22,0x41 }, // <
    { 0x14,0x14,0x14,0x14,0x14 }, // =
    { 0x41,0x22,0x14,0x08,0x00 }, // >
    { 0x02,0x01,0x51,0x09,0x06 }, // ?
    { 0x32,0x49,0x79,0x41,0x3E }, // @
    { 0x7E,0x11,0x11,0x11,0x7E }, // A
    { 0x7F,0x49,0x49,0x49,0x36 }, // B
    { 0x3E,0x41,0x41,0x41,0x22 }, // C
    { 0x7F,0x41,0x41,0x22,0x1C }, // D
    { 0x7F,0x49,0x49,0x49,0x41 }, // E
    { 0x7F,0x09,0x09,0x09,0x01 }, // F
    { 0x3E,0x41,0x49,0x49,0x7A }, // G
    { 0x7F,0x08,0x08,0x08,0x7F }, // H
    { 0x00,0x41,0x7F,0x41,0x00 }, // I
    { 0x20,0x40,0x41,0x3F,0x01 }, // J
    { 0x7F,0x08,0x14,0x22,0x41 }, // K
    { 0x7F,0x40,0x40,0x40,0x40 }, // L
    { 0x7F,0x02,0x04,0x02,0x7F }, // M
    { 0x7F,0x04,0x08,0x10,0x7F }, // N
    { 0x3E,0x41,0x41,0x41,0x3E }, // O
    { 0x7F,0x09,0x09,0x09,0x06 }, // P
    { 0x3E,0x41,0x51,0x21,0x5E }, // Q
    { 0x7F,0x09,0x19,0x29,0x46 }, // R
    { 0x46,0x49,0x49,0x49,0x31 }, // S
    { 0x01,0x01,0x7F,0x01,0x01 }, // T
    { 0x3F,0x40,0x40,0x40,0x3F }, // U
    { 0x1F,0x20,0x40,0x20,0x1F }, // V
    { 0x3F,0x40,0x38,0x40,0x3F }, // W
    { 0x63,0x14,0x08,0x14,0x63 }, // X
    { 0x07,0x08,0x70,0x08,0x07 }, // Y
    { 0x61,0x51,0x49,0x45,0x43 }, // Z
    { 0x00,0x7F,0x41,0x41,0x00 }, // [
    { 0x02,0x04,0x08,0x10,0x20 }, // backslash
    { 0x00,0x41,0x41,0x7F,0x00 }, // ]
    { 0x04,0x02,0x01,0x02,0x04 }, // ^
    { 0x40,0x40,0x40,0x40,0x40 }, // _
    { 0x00,0x01,0x02,0x04,0x00 }, // `
    { 0x20,0x54,0x54,0x54,0x78 }, // a
    { 0x7F,0x48,0x44,0x44,0x38 }, // b
    { 0x38,0x44,0x44,0x44,0x20 }, // c
    { 0x38,0x44,0x44,0x48,0x7F }, // d
    { 0x38,0x54,0x54,0x54,0x18 }, // e
    { 0x08,0x7E,0x09,0x01,0x02 }, // f
    { 0x0C,0x52,0x52,0x52,0x3E }, // g
    { 0x7F,0x08,0x04,0x04,0x78 }, // h
    { 0x00,0x44,0x7D,0x40,0x00 }, // i
    { 0x20,0x40,0x44,0x3D,0x00 }, // j
    { 0x7F,0x10,0x28,0x44,0x00 }, // k
    { 0x00,0x41,0x7F,0x40,0x00 }, // l
    { 0x7C,0x04,0x18,0x04,0x78 }, // m
    { 0x7C,0x08,0x04,0x04,0x78 }, // n
    { 0x38,0x44,0x44,0x44,0x38 }, // o
    { 0x7C,0x14,0x14,0x14,0x08 }, // p
    { 0x08,0x14,0x14,0x18,0x7C }, // q
    { 0x7C,0x08,0x04,0x04,0x08 }, // r
    { 0x48,0x54,0x54,0x54,0x20 }, // s
    { 0x04,0x3F,0x44,0x40,0x20 }, // t
    { 0x3C,0x40,0x40,0x20,0x7C }, // u
    { 0x1C,0x20,0x40,0x20,0x1C }, // v
    { 0x3C,0x40,0x30,0x40,0x3C }, // w
    { 0x44,0x28,0x10,0x28,0x44 }, // x
    { 0x0C,0x50,0x50,0x50,0x3C }, // y
    { 0x44,0x64,0x54,0x4C,0x44 }, // z
    { 0x00,0x08,0x36,0x41,0x00 }, // {
    { 0x00,0x00,0x7F,0x00,0x00 }, // |
    { 0x00,0x41,0x36,0x08,0x00 }, // }
    { 0x08,0x08,0x2A,0x1C,0x08 }, // ->
    { 0x08,0x1C,0x2A,0x08,0x08 }, // <-
};

// Character cell: 6 wide (5 font + 1 spacing), 8 tall
#define CHAR_W 6
#define CHAR_H 8

// ---------------------------------------------------------------------------
// Init command sequences
// ---------------------------------------------------------------------------
static const uint8_t INIT_64[] = {
    0xAE,        // display off
    0xD5, 0x80,  // clock divide / osc freq
    0xA8, 0x3F,  // multiplex 1/64
    0xD3, 0x00,  // display offset 0
    0x40,        // start line 0
    0x8D, 0x14,  // charge pump on
    0x20, 0x00,  // horizontal addressing mode
    0xA1,        // segment remap (mirror horizontal)
    0xC8,        // COM scan direction remapped
    0xDA, 0x12,  // COM pins hardware config (alt, no remap)
    0x81, 0xCF,  // contrast
    0xD9, 0xF1,  // pre-charge period
    0xDB, 0x40,  // VCOMH deselect level
    0xA4,        // display from RAM
    0xA6,        // normal (not inverted)
    0xAF,        // display on
};

static const uint8_t INIT_32[] = {
    0xAE,
    0xD5, 0x80,
    0xA8, 0x1F,  // multiplex 1/32
    0xD3, 0x00,
    0x40,
    0x8D, 0x14,
    0x20, 0x00,
    0xA1,
    0xC8,
    0xDA, 0x02,  // COM pins: sequential, no remap (32-pixel panels)
    0x81, 0x8F,
    0xD9, 0xF1,
    0xDB, 0x40,
    0xA4,
    0xA6,
    0xAF,
};

static void send_cmd(SSD1306 *d, uint8_t c) {
    uint8_t buf[2] = { 0x00, c }; // Co=0, D/C=0
    i2c_write_blocking(d->i2c, d->addr, buf, 2, false);
}

static void send_cmds(SSD1306 *d, const uint8_t *cmds, size_t len) {
    for (size_t i = 0; i < len; i++) send_cmd(d, cmds[i]);
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void ssd1306_init(SSD1306 *d, i2c_inst_t *i2c, uint8_t addr, uint8_t height) {
    d->i2c    = i2c;
    d->addr   = addr;
    d->height = height;
    memset(d->fb, 0, sizeof(d->fb));

    if (height == SSD1306_H32)
        send_cmds(d, INIT_32, sizeof(INIT_32));
    else
        send_cmds(d, INIT_64, sizeof(INIT_64));

    ssd1306_flush(d);
}

// ---------------------------------------------------------------------------
// Flush framebuffer to display
// ---------------------------------------------------------------------------
void ssd1306_flush(SSD1306 *d) {
    uint8_t pages = d->height / 8;

    // Set column and page address to full range
    send_cmd(d, 0x21); send_cmd(d, 0); send_cmd(d, 127);         // col 0-127
    send_cmd(d, 0x22); send_cmd(d, 0); send_cmd(d, pages - 1);   // page 0..N

    // Send the entire framebuffer in one transaction with data prefix byte.
    // I2C write: [0x40, fb[0], fb[1], ...]
    // We chunk in 128-byte writes (one page each) with the 0x40 prefix.
    uint16_t fb_bytes = SSD1306_WIDTH * pages;
    uint8_t  chunk[SSD1306_WIDTH + 1];
    chunk[0] = 0x40; // D/C = data

    for (uint8_t page = 0; page < pages; page++) {
        memcpy(chunk + 1, d->fb + page * SSD1306_WIDTH, SSD1306_WIDTH);
        i2c_write_blocking(d->i2c, d->addr, chunk, SSD1306_WIDTH + 1, false);
    }
    (void)fb_bytes;
}

// ---------------------------------------------------------------------------
// Drawing primitives
// ---------------------------------------------------------------------------
void ssd1306_fill(SSD1306 *d, uint8_t color) {
    memset(d->fb, color ? 0xFF : 0x00, SSD1306_WIDTH * d->height / 8);
}

void ssd1306_clear(SSD1306 *d) {
    ssd1306_fill(d, 0);
    ssd1306_flush(d);
}

void ssd1306_draw_pixel(SSD1306 *d, int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= d->height) return;
    uint16_t byte_idx = (uint16_t)(x + (y / 8) * SSD1306_WIDTH);
    uint8_t  bit      = 1 << (y & 7);
    if (color) d->fb[byte_idx] |=  bit;
    else       d->fb[byte_idx] &= ~bit;
}

void ssd1306_draw_hline(SSD1306 *d, int16_t x, int16_t y, int16_t w, uint8_t color) {
    for (int16_t i = 0; i < w; i++) ssd1306_draw_pixel(d, x + i, y, color);
}

void ssd1306_draw_vline(SSD1306 *d, int16_t x, int16_t y, int16_t h, uint8_t color) {
    for (int16_t i = 0; i < h; i++) ssd1306_draw_pixel(d, x, y + i, color);
}

void ssd1306_draw_rect(SSD1306 *d, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    ssd1306_draw_hline(d, x,         y,         w, color);
    ssd1306_draw_hline(d, x,         y + h - 1, w, color);
    ssd1306_draw_vline(d, x,         y,         h, color);
    ssd1306_draw_vline(d, x + w - 1, y,         h, color);
}

void ssd1306_fill_rect(SSD1306 *d, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    for (int16_t row = 0; row < h; row++)
        ssd1306_draw_hline(d, x, y + row, w, color);
}

void ssd1306_draw_round_rect(SSD1306 *d, int16_t x, int16_t y, int16_t w, int16_t h,
                               int16_t r, uint8_t color) {
    // Draw flat sides
    ssd1306_draw_hline(d, x + r, y,         w - 2 * r, color);
    ssd1306_draw_hline(d, x + r, y + h - 1, w - 2 * r, color);
    ssd1306_draw_vline(d, x,         y + r, h - 2 * r, color);
    ssd1306_draw_vline(d, x + w - 1, y + r, h - 2 * r, color);
    // Corners — Bresenham circle quadrants
    int16_t cx, cy, f, ddF_x, ddF_y;
    for (int q = 0; q < 4; q++) {
        cx = 0; cy = r; f = 1 - r; ddF_x = 1; ddF_y = -2 * r;
        int16_t ox = (q & 1) ? (x + r) : (x + w - r - 1);
        int16_t oy = (q & 2) ? (y + r) : (y + h - r - 1);
        int16_t sx = (q & 1) ? -1 : 1;
        int16_t sy = (q & 2) ? -1 : 1;
        while (cx <= cy) {
            ssd1306_draw_pixel(d, ox + sx * cy, oy + sy * cx, color);
            ssd1306_draw_pixel(d, ox + sx * cx, oy + sy * cy, color);
            if (f >= 0) { cy--; ddF_y += 2; f += ddF_y; }
            cx++; ddF_x += 2; f += ddF_x;
        }
    }
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------
int16_t ssd1306_draw_char(SSD1306 *d, int16_t x, int16_t y, char c,
                           uint8_t color, uint8_t scale) {
    if (c < 0x20 || c > 0x7F) c = '?';
    const uint8_t *glyph = FONT5X7[c - 0x20];
    for (int col = 0; col < 5; col++) {
        uint8_t column = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (column & (1 << row)) {
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        ssd1306_draw_pixel(d,
                            x + col * scale + sx,
                            y + row * scale + sy,
                            color);
            }
        }
    }
    // Spacing column
    for (int row = 0; row < 7 * scale; row++)
        for (int sx = 0; sx < scale; sx++)
            ssd1306_draw_pixel(d, x + 5 * scale + sx, y + row, 0);

    return x + CHAR_W * scale;
}

int16_t ssd1306_draw_str(SSD1306 *d, int16_t x, int16_t y, const char *s,
                          uint8_t color, uint8_t scale) {
    while (*s) {
        x = ssd1306_draw_char(d, x, y, *s++, color, scale);
        if (x >= SSD1306_WIDTH) break;
    }
    return x;
}

int16_t ssd1306_str_width(const char *s, uint8_t scale) {
    int16_t w = 0;
    while (*s++) w += CHAR_W * scale;
    return w;
}

void ssd1306_draw_str_aligned(SSD1306 *d, int16_t x_start, int16_t x_end,
                               int16_t y, const char *s,
                               uint8_t color, uint8_t scale, TextAlign align) {
    int16_t tw = ssd1306_str_width(s, scale);
    int16_t span = x_end - x_start;
    int16_t x;
    switch (align) {
    case ALIGN_CENTER: x = x_start + (span - tw) / 2; break;
    case ALIGN_RIGHT:  x = x_end - tw;                break;
    default:           x = x_start;                   break;
    }
    ssd1306_draw_str(d, x, y, s, color, scale);
}

// ---------------------------------------------------------------------------
// Battery graphics — ported from DisplayManager::drawBattery*()
//
// Each function:
//   1. Clears the framebuffer
//   2. Draws a hollow body rect + filled positive terminal on the right
//   3. Draws a centred text label inside the body
// Caller must call ssd1306_flush() to push to the display.
// ---------------------------------------------------------------------------

void ssd1306_draw_battery_one_aa(SSD1306 *d) {
    ssd1306_fill(d, 0);

    int16_t cx = SSD1306_WIDTH / 2;
    int16_t cy = d->height / 2;

    // Body: 44 × 18
    const int16_t bw = 44, bh = 18;
    // Positive terminal: 4 × 8, flush against right edge of body
    const int16_t tw = 4,  th = 8;
    int16_t bx = cx - (bw + tw) / 2;
    int16_t by = cy - bh / 2;

    ssd1306_draw_rect(d, bx, by, bw, bh, 1);
    ssd1306_fill_rect(d, bx + bw, by + (bh - th) / 2, tw, th, 1);

    // "AA" label — scale 2 (12 × 14 px), centred inside body
    int16_t lw = ssd1306_str_width("AA", 2);
    ssd1306_draw_str(d, bx + (bw - lw) / 2, cy - 7, "AA", 1, 2);
}

void ssd1306_draw_battery_two_aa(SSD1306 *d) {
    ssd1306_fill(d, 0);

    // Two AA batteries, slightly smaller, stacked vertically
    const int16_t bw = 44, bh = 12;
    const int16_t tw = 4,  th = 6;
    int16_t cx = SSD1306_WIDTH / 2;
    int16_t bx = cx - (bw + tw) / 2;

    // y offsets relative to display centre (top battery above, bottom below)
    int16_t offsets[2] = { -8, 6 };
    for (int i = 0; i < 2; i++) {
        int16_t by = d->height / 2 + offsets[i] - bh / 2;
        ssd1306_draw_rect(d, bx, by, bw, bh, 1);
        ssd1306_fill_rect(d, bx + bw, by + (bh - th) / 2, tw, th, 1);
    }

    // "AA x2" label — scale 1 (tight fit between the two batteries)
    int16_t lw = ssd1306_str_width("AA x2", 1);
    ssd1306_draw_str(d, bx + (bw - lw) / 2, d->height / 2 - 4, "AA x2", 1, 1);
}

void ssd1306_draw_battery_one_d(SSD1306 *d) {
    ssd1306_fill(d, 0);

    int16_t cx = SSD1306_WIDTH / 2;
    int16_t cy = d->height / 2;

    // Body: 36 × 26 (D cell is fatter than AA)
    const int16_t bw = 36, bh = 26;
    const int16_t tw = 5,  th = 10;
    int16_t bx = cx - (bw + tw) / 2;
    int16_t by = cy - bh / 2;

    ssd1306_draw_rect(d, bx, by, bw, bh, 1);
    ssd1306_fill_rect(d, bx + bw, by + (bh - th) / 2, tw, th, 1);

    // "D" label — scale 2, centred inside body
    int16_t lw = ssd1306_str_width("D", 2);
    ssd1306_draw_str(d, bx + (bw - lw) / 2, cy - 7, "D", 1, 2);
}
