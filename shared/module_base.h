#pragma once

#include "pico/stdlib.h"
#include "bus_protocol.h"

// ---------------------------------------------------------------------------
// Standard pin assignments — same on every module Pico
// Change the defines here if your wiring differs; logic never hardcodes pins.
//
// RESERVED BY THE INTER-PICO BUS (see bus_protocol.h — do not reuse):
//   GP2  I2C1 SDA   bus data
//   GP3  I2C1 SCL   bus clock
//   GP6  ATTN       shared open-drain "I have news" line
// ---------------------------------------------------------------------------

// Status RGB LED (common cathode; drive pin HIGH = LED on)
#define STATUS_LED_R_PIN    10  // GP10, physical pin 14
#define STATUS_LED_G_PIN    11  // GP11, physical pin 15
#define STATUS_LED_B_PIN    12  // GP12, physical pin 16

// I2C0 — this module's OWN peripherals (OLEDs, TCA9548A, HT16K33).
// Completely separate from the inter-Pico bus on i2c1.
#define I2C_BUS             i2c0
#define I2C_SDA_PIN         4   // GP4, physical pin 6
#define I2C_SCL_PIN         5   // GP5, physical pin 7
#define I2C_FREQ_HZ         400000

// SPI0 for SK9822 addressable LED strips (on modules that use them).
// Moved off GP2/GP3 — those are now the inter-Pico I2C bus.
#define LED_STRIP_SPI       spi0
#define LED_STRIP_CLK_PIN   18  // GP18, physical pin 24 (SPI0 SCK)
#define LED_STRIP_DATA_PIN  19  // GP19, physical pin 25 (SPI0 TX / MOSI)

// ---------------------------------------------------------------------------
// Module lifecycle states
// ---------------------------------------------------------------------------
typedef enum {
    MOD_IDLE,       // Waiting for MSG_INIT
    MOD_READY,      // Initialised, waiting for MSG_START
    MOD_ACTIVE,     // Running — player can interact
    MOD_SOLVED,     // Defused successfully
    MOD_LOCKED,     // Central locked this module for this level
    MOD_EXPLODED,   // Game over (lose)
} ModuleLifecycle;

// ---------------------------------------------------------------------------
// Init payload (MSG_INIT) — central sends this once per game
// Packed struct; must match central_controller/game_state.h GameInitPayload.
// 12 bytes, fits inside PROTO_MAX_PAYLOAD (16).
// ---------------------------------------------------------------------------
typedef struct __attribute__((packed)) {
    uint8_t  level;
    uint8_t  serial[6];    // ASCII, e.g. "A3B9X2"
    uint8_t  battery_aa;   // 0–2
    uint8_t  battery_d;    // 0–1
    uint16_t indicators;   // bitmask — IND_* defines in bus_protocol.h
    uint8_t  repeat_count; // for multi-round modules (Memory, Who's on First)
} GameInitPayload;

// IND_* defines are in bus_protocol.h (included above).

// ---------------------------------------------------------------------------
// Module context — embed one of these in each module's global state
// ---------------------------------------------------------------------------
typedef struct {
    uint8_t          my_id;
    ModuleLifecycle  lifecycle;
    GameInitPayload  game;          // filled on MSG_INIT
} ModuleCtx;

// ---------------------------------------------------------------------------
// Call these from your module's main()
// ---------------------------------------------------------------------------

// Initialise the I2C slave bus, ATTN line, and status LED GPIO.
void module_init(ModuleCtx *ctx, uint8_t my_id);

// Call once per main-loop iteration. Drains the bus inbox and dispatches to
// your callbacks. Returns true if a message was processed.
bool module_poll(ModuleCtx *ctx);

// ---------------------------------------------------------------------------
// Callbacks — implement these in your module's main.c
// ---------------------------------------------------------------------------

// Called when the central sends MSG_INIT.  Use ctx->game for parameters.
void on_module_init(ModuleCtx *ctx);

// Called when MSG_START arrives — begin accepting player input.
void on_module_start(ModuleCtx *ctx);

// Called on MSG_STOP (win or lose).  reason is STOP_WIN/STOP_LOSE/STOP_RESET.
void on_module_stop(ModuleCtx *ctx, uint8_t reason);

// Called on MSG_RESET — return hardware to idle state.
void on_module_reset(ModuleCtx *ctx);

// ---------------------------------------------------------------------------
// Helpers your module calls to notify the central controller.
// These queue a frame and raise ATTN; the central collects it on its next
// sweep (typically within a few milliseconds).
// ---------------------------------------------------------------------------
void module_send_solved(ModuleCtx *ctx);
void module_send_strike(ModuleCtx *ctx);
void module_send_ready(ModuleCtx *ctx);

// ---------------------------------------------------------------------------
// Status LED helpers
// ---------------------------------------------------------------------------
void status_led_set(uint8_t r, uint8_t g, uint8_t b);
void status_led_idle(void);     // white dim
void status_led_active(void);   // blue
void status_led_solved(void);   // green
void status_led_strike(void);   // red flash (non-blocking, call from loop)
void status_led_locked(void);   // off

// ---------------------------------------------------------------------------
// ROSC-based random (Pico-specific; no external seed needed)
// ---------------------------------------------------------------------------
uint32_t rosc_random32(void);
