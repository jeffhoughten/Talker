#pragma once

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ---------------------------------------------------------------------------
// SSD1306 OLED driver — supports both 128×64 and 128×32 panels.
// Pure I2C, no DMA.  Caller selects the TCA9548A channel before calling.
// ---------------------------------------------------------------------------

#define SSD1306_ADDR        0x3C
#define SSD1306_WIDTH       128

// Pass one of these as height when initialising
#define SSD1306_H64         64
#define SSD1306_H32         32

// Framebuffer sizes
#define SSD1306_FB_64  (SSD1306_WIDTH * SSD1306_H64 / 8)  // 1024 bytes
#define SSD1306_FB_32  (SSD1306_WIDTH * SSD1306_H32 / 8)  // 512 bytes
#define SSD1306_FB_MAX SSD1306_FB_64

// Text alignment
typedef enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT } TextAlign;

// ---------------------------------------------------------------------------
// Context
// ---------------------------------------------------------------------------
typedef struct {
    i2c_inst_t *i2c;
    uint8_t     addr;
    uint8_t     height;
    uint8_t     fb[SSD1306_FB_MAX];
} SSD1306;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void ssd1306_init(SSD1306 *d, i2c_inst_t *i2c, uint8_t addr, uint8_t height);
void ssd1306_flush(SSD1306 *d);   // send framebuffer to display
void ssd1306_clear(SSD1306 *d);   // fill with 0 then flush

// ---------------------------------------------------------------------------
// Drawing (all modify the framebuffer; call ssd1306_flush to push to screen)
// ---------------------------------------------------------------------------
void ssd1306_fill(SSD1306 *d, uint8_t color);       // 0=black, 1=white
void ssd1306_draw_pixel(SSD1306 *d, int16_t x, int16_t y, uint8_t color);
void ssd1306_draw_hline(SSD1306 *d, int16_t x, int16_t y, int16_t w, uint8_t color);
void ssd1306_draw_vline(SSD1306 *d, int16_t x, int16_t y, int16_t h, uint8_t color);
void ssd1306_draw_rect(SSD1306 *d, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void ssd1306_fill_rect(SSD1306 *d, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void ssd1306_draw_round_rect(SSD1306 *d, int16_t x, int16_t y, int16_t w, int16_t h,
                              int16_t r, uint8_t color);

// ---------------------------------------------------------------------------
// Text (5×7 font, 6×8 character cell including 1 pixel spacing)
// ---------------------------------------------------------------------------

// Draw a single character at pixel position (x, y). Returns new x.
int16_t ssd1306_draw_char(SSD1306 *d, int16_t x, int16_t y, char c,
                           uint8_t color, uint8_t scale);

// Draw a string at (x, y). Returns new x after last character.
int16_t ssd1306_draw_str(SSD1306 *d, int16_t x, int16_t y, const char *s,
                          uint8_t color, uint8_t scale);

// Measure string pixel width at given scale (for centering).
int16_t ssd1306_str_width(const char *s, uint8_t scale);

// ---------------------------------------------------------------------------
// Convenience: draw a string centered or right-aligned within a given area.
// x_start/x_end define the horizontal span; y is the top of the text.
// ---------------------------------------------------------------------------
void ssd1306_draw_str_aligned(SSD1306 *d, int16_t x_start, int16_t x_end,
                               int16_t y, const char *s,
                               uint8_t color, uint8_t scale, TextAlign align);

// ---------------------------------------------------------------------------
// Battery graphics — each function clears the framebuffer, draws the shape
// centred on the display, and labels it.  Call ssd1306_flush() after.
//
// Layout (matches DisplayManager::drawBattery* in the original code):
//   Body : hollow rect, positive terminal as filled rect on the right side.
//   Label: text centred inside the body.
// ---------------------------------------------------------------------------
void ssd1306_draw_battery_one_aa(SSD1306 *d);   // single AA  — 128×64 panel
void ssd1306_draw_battery_two_aa(SSD1306 *d);   // two AA     — 128×64 panel
void ssd1306_draw_battery_one_d(SSD1306 *d);    // single D   — 128×64 panel
