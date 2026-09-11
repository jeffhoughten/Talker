#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdio.h>

#include "../shared/bus_protocol.h"
#include "../shared/module_base.h"
#include "game_state.h"
#include "level_config.h"
#include "ht16k33.h"
#include "ssd1306.h"
#include "save.h"

// ---------------------------------------------------------------------------
// Central controller pin assignments
//
// The central is the I2C MASTER of the inter-Pico bus (i2c1, GP2/GP3) and
// watches the shared ATTN line on GP6. See shared/bus_protocol.h.
// Its own displays live on a completely separate bus (i2c0, GP4/GP5).
// ---------------------------------------------------------------------------
#define BIG_BUTTON_PIN      20  // GP20, physical pin 26 — momentary button
#define BUTTON_GND_PIN      21  // GP21, physical pin 27 — driven LOW as button ground
                                // (moved off GP18, now SPI0 SCK for the LED strip)

// WS2812 strip (Big Button color ring, 12 LEDs) — driven via PIO
#define WS2812_PIN          15  // GP15, physical pin 20

// SK9822 strip (Big Button accent strip, 11 LEDs) — driven via SPI0
// Uses LED_STRIP_CLK_PIN / LED_STRIP_DATA_PIN from module_base.h (GP18/GP19)

// Central I2C bus for TCA9548A and displays — same as modules
// Uses I2C_SDA_PIN / I2C_SCL_PIN from module_base.h (GP4/GP5)

// TCA9548A channel assignments for the central controller displays.
// Fixed channels never move between games.
#define TCA_CH_TIMER        0   // 1.2" 7-seg HT16K33  — always ch 0
#define TCA_CH_STRIKES      4   // 128x64 OLED strikes  — always ch 4
#define TCA_CH_BIG_BUTTON   6   // 128x64 OLED Big Button label — always ch 6

// Randomised channels — swapped between the two slots each game start.
// Serial  ↔  Indicator : ch 3 or ch 7  (both 128x32)
// Batt 1  ↔  Batt 2    : ch 1 or ch 2  (both 128x64)
// Runtime variables; initialised by displays_randomize_channels().
static uint8_t tca_ch_serial;    // 3 or 7
static uint8_t tca_ch_indicator; // the other of {3, 7}
static uint8_t tca_ch_batt1;     // 1 or 2
static uint8_t tca_ch_batt2;     // the other of {1, 2}

// TCA9548A I2C address
#define TCA_ADDR            0x70
// Display I2C address (all on same addr, mux selects which one is active)
#define DISPLAY_ADDR        0x3C
// 7-seg HT16K33 address
#define SEVENSEG_ADDR       0x71  // A0 bridged on backpack

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static GameState gs;
static SaveData  save;

// Display instances — one per TCA9548A channel
static HT16K33 disp_timer;       // ch 0 — 7-segment timer
static SSD1306 disp_batt1;       // 128×64 battery panel 1 (ch 1 or 2, randomised)
static SSD1306 disp_batt2;       // 128×64 battery panel 2 (ch 1 or 2, randomised)
static SSD1306 disp_serial;      // 128×32 serial number   (ch 3 or 7, randomised)
static SSD1306 disp_strikes;     // ch 4 — 128×64 strikes  (fixed)
static SSD1306 disp_bigbutton;   // ch 6 — 128×64 Big Button label (fixed)
static SSD1306 disp_indicator;   // 128×32 indicator       (ch 3 or 7, randomised)

// ---------------------------------------------------------------------------
// I2C / TCA helpers
// ---------------------------------------------------------------------------
static void tca_select(uint8_t channel) {
    if (channel > 7) return;
    uint8_t buf = (1 << channel);
    i2c_write_blocking(I2C_BUS, TCA_ADDR, &buf, 1, false);
}

static void tca_select_none(void) {
    uint8_t buf = 0;
    i2c_write_blocking(I2C_BUS, TCA_ADDR, &buf, 1, false);
}

// ---------------------------------------------------------------------------
// End-state display helpers
// Scroll a message across the 4-digit 7-seg.  Call once per loop iteration;
// advances the window every SCROLL_INTERVAL_MS milliseconds.
// Returns true when the full message has scrolled off the right side.
// ---------------------------------------------------------------------------
#define SCROLL_INTERVAL_MS  350

typedef struct {
    const char *msg;
    uint8_t     pos;        // current window start index into padded string
    uint32_t    next_ms;    // absolute ms timestamp of next advance
    bool        done;
} ScrollState;

static void scroll_start(ScrollState *s, const char *msg) {
    s->msg     = msg;
    s->pos     = 0;
    s->next_ms = to_ms_since_boot(get_absolute_time());
    s->done    = false;
}

static bool scroll_tick(ScrollState *s) {
    if (s->done) return true;

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now < s->next_ms) return false;
    s->next_ms = now + SCROLL_INTERVAL_MS;

    // Build a 4-char window into "    <msg>    " so the text scrolls in from
    // the right and fully off to the left before done is set.
    uint8_t msg_len = (uint8_t)strlen(s->msg);
    uint8_t total   = msg_len + 8;  // 4 leading + 4 trailing blank positions

    char window[5];
    for (int i = 0; i < 4; i++) {
        int idx = (int)s->pos + i - 4;
        window[i] = (idx >= 0 && idx < msg_len) ? s->msg[idx] : ' ';
    }
    window[4] = '\0';

    tca_select(TCA_CH_TIMER);
    ht16k33_print_str(&disp_timer, window);
    tca_select_none();

    s->pos++;
    if (s->pos >= total) s->done = true;
    return s->done;
}

// ---------------------------------------------------------------------------
// Display helpers — each selects its TCA channel, updates, then deselects
// ---------------------------------------------------------------------------

static void display_update_timer(uint32_t remaining_ms) {
    tca_select(TCA_CH_TIMER);
    ht16k33_show_time(&disp_timer, remaining_ms);
    tca_select_none();
}

static void display_update_strikes(uint8_t strikes) {
    tca_select(TCA_CH_STRIKES);
    ssd1306_fill(&disp_strikes, 0);

    // Large centred strike count
    char buf[4];
    snprintf(buf, sizeof(buf), "%u", strikes);
    ssd1306_draw_str_aligned(&disp_strikes, 0, SSD1306_WIDTH,
                              8, buf, 1, 4, ALIGN_CENTER);

    // Small "STRIKES" label at top
    ssd1306_draw_str_aligned(&disp_strikes, 0, SSD1306_WIDTH,
                              0, "STRIKES", 1, 1, ALIGN_CENTER);
    ssd1306_flush(&disp_strikes);
    tca_select_none();
}

static void display_show_serial(const char *serial) {
    tca_select(tca_ch_serial);
    ssd1306_fill(&disp_serial, 0);
    // Serial is 6 chars; scale 2 = 12px tall, fits 128×32 with room
    ssd1306_draw_str_aligned(&disp_serial, 0, SSD1306_WIDTH,
                              8, serial, 1, 2, ALIGN_CENTER);
    ssd1306_flush(&disp_serial);
    tca_select_none();
}

// display_show_batteries() and display_show_indicator() have been removed.
// Battery and indicator rendering is now handled inside scenario_randomize(),
// which draws per-slot content (BATT_ONE_AA, BATT_TWO_AA, BATT_ONE_D) rather
// than aggregate counts, and places them on the randomly assigned channels.

static void display_set_big_button_label(const char *text) {
    tca_select(TCA_CH_BIG_BUTTON);
    ssd1306_fill(&disp_bigbutton, 0);
    // Vertically centre on 64px panel: scale 2 = 14px glyph height, top at y=25
    ssd1306_draw_str_aligned(&disp_bigbutton, 0, SSD1306_WIDTH,
                              25, text, 1, 2, ALIGN_CENTER);
    ssd1306_flush(&disp_bigbutton);
    tca_select_none();
}

// Initialise all displays (call once from main after I2C is up).
// Randomise channels first, then call this.
static void displays_init(void) {
    tca_select(TCA_CH_TIMER);
    ht16k33_init(&disp_timer,    I2C_BUS, HT16K33_ADDR);
    tca_select(tca_ch_batt1);
    ssd1306_init(&disp_batt1,    I2C_BUS, SSD1306_ADDR, SSD1306_H64);
    tca_select(tca_ch_batt2);
    ssd1306_init(&disp_batt2,    I2C_BUS, SSD1306_ADDR, SSD1306_H64);
    tca_select(tca_ch_serial);
    ssd1306_init(&disp_serial,   I2C_BUS, SSD1306_ADDR, SSD1306_H32);
    tca_select(TCA_CH_STRIKES);
    ssd1306_init(&disp_strikes,  I2C_BUS, SSD1306_ADDR, SSD1306_H64);
    tca_select(TCA_CH_BIG_BUTTON);
    ssd1306_init(&disp_bigbutton,I2C_BUS, SSD1306_ADDR, SSD1306_H64);
    tca_select(tca_ch_indicator);
    ssd1306_init(&disp_indicator,I2C_BUS, SSD1306_ADDR, SSD1306_H32);
    tca_select_none();
}

// Randomly assign Serial↔Indicator to {ch3, ch7} and Batt1↔Batt2 to {ch1, ch2}.
// Call this at each game start (before displays_init or the re-init path below).
// After calling, re-init the four randomised displays so their SSD1306 init
// sequence reaches the correct physical panel.
static void displays_randomize_channels(void) {
    uint32_t r = rosc_random32();

    // Serial vs Indicator
    if (r & 0x01) {
        tca_ch_serial    = 3;
        tca_ch_indicator = 7;
    } else {
        tca_ch_serial    = 7;
        tca_ch_indicator = 3;
    }

    // Battery panel 1 vs 2
    if (r & 0x02) {
        tca_ch_batt1 = 1;
        tca_ch_batt2 = 2;
    } else {
        tca_ch_batt1 = 2;
        tca_ch_batt2 = 1;
    }

    // Re-send init sequence to each of the four randomised displays so the
    // physical panel on the newly assigned channel wakes up correctly.
    tca_select(tca_ch_batt1);
    ssd1306_init(&disp_batt1,    I2C_BUS, SSD1306_ADDR, SSD1306_H64);
    tca_select(tca_ch_batt2);
    ssd1306_init(&disp_batt2,    I2C_BUS, SSD1306_ADDR, SSD1306_H64);
    tca_select(tca_ch_serial);
    ssd1306_init(&disp_serial,   I2C_BUS, SSD1306_ADDR, SSD1306_H32);
    tca_select(tca_ch_indicator);
    ssd1306_init(&disp_indicator,I2C_BUS, SSD1306_ADDR, SSD1306_H32);
    tca_select_none();
}

// ---------------------------------------------------------------------------
// Bomb scenario — batteries, indicator, Big Button setup
// Generated fresh each game start by scenario_randomize().
// ---------------------------------------------------------------------------

// What a single battery display slot shows
typedef enum {
    BATT_EMPTY  = 0,  // nothing on this screen
    BATT_ONE_AA = 1,
    BATT_TWO_AA = 2,
    BATT_ONE_D  = 3,
} BatterySlot;

typedef struct {
    uint8_t     bb_color;        // BB_COLOR_* — button ring colour
    uint8_t     bb_text_idx;     // index into BUTTONTEXT[]
    uint8_t     indicator_idx;   // index into INDICATORS[] (0 = blank)
    BatterySlot batt1;           // content of battery screen 1 (tca_ch_batt1)
    BatterySlot batt2;           // content of battery screen 2 (tca_ch_batt2)
} BombScenario;

static BombScenario scenario;

// Big Button color constants (WS2812 fill colors)
#define BB_COLOR_RED    0
#define BB_COLOR_BLUE   1
#define BB_COLOR_YELLOW 2
#define BB_COLOR_WHITE  3
#define BB_NUM_COLORS   4

static const char *BUTTONTEXT[] = { "Press", "", "Abort", "Detonate", "Hold" };
#define BUTTONTEXT_COUNT 5

static const char *INDICATORS[] = {
    "",    "SND", "CLR", "IND", "FRQ", "SIG",
    "NSA", "MSA", "TRN", "BOB", "CAR", "FRK"
};
#define INDICATOR_COUNT   12
#define INDICATOR_IDX_FRK 11

// Map a BatterySlot pair to total AA / D counts for init_payload
static void battery_slots_to_counts(BatterySlot b1, BatterySlot b2,
                                     uint8_t *aa_out, uint8_t *d_out) {
    uint8_t aa = 0, d = 0;
    BatterySlot slots[2] = { b1, b2 };
    for (int i = 0; i < 2; i++) {
        switch (slots[i]) {
        case BATT_ONE_AA: aa += 1; break;
        case BATT_TWO_AA: aa += 2; break;
        case BATT_ONE_D:  d  += 1; break;
        default: break;
        }
    }
    *aa_out = aa;
    *d_out  = d;
}

// ---------------------------------------------------------------------------
// scenario_randomize()
// Ported directly from the original ModuleState.cpp Big Button setup logic.
// Sets scenario.*, updates g->init_payload battery and indicator fields,
// and renders the battery and indicator displays.
// Call after game_load_level() and displays_randomize_channels().
// ---------------------------------------------------------------------------
static void scenario_randomize(GameState *g) {
    uint32_t randomizer;

    randomizer = rosc_random32();
    if (randomizer % 2 == 0) {
        // ----------------------------------------------------------------
        // IMMEDIATE RELEASE branch — three sub-cases
        // ----------------------------------------------------------------
        randomizer = rosc_random32();
        switch (randomizer % 3) {

        case 0: {
            // Random color; "Detonate" text; random indicator; random batteries
            randomizer = rosc_random32();
            scenario.bb_color    = randomizer % BB_NUM_COLORS;
            scenario.bb_text_idx = 3;  // "Detonate"

            randomizer = rosc_random32();
            scenario.indicator_idx = randomizer % INDICATOR_COUNT;

            randomizer = rosc_random32();
            switch (randomizer % 6) {
            case 0: scenario.batt1 = BATT_TWO_AA; scenario.batt2 = BATT_EMPTY;  break;
            case 1: scenario.batt1 = BATT_EMPTY;  scenario.batt2 = BATT_TWO_AA; break;
            case 2: scenario.batt1 = BATT_TWO_AA; scenario.batt2 = BATT_ONE_D;  break;
            case 3: scenario.batt1 = BATT_ONE_D;  scenario.batt2 = BATT_TWO_AA; break;
            case 4: scenario.batt1 = BATT_ONE_D;  scenario.batt2 = BATT_ONE_AA; break;
            case 5: scenario.batt1 = BATT_ONE_AA; scenario.batt2 = BATT_ONE_D;  break;
            }
            break;
        }

        case 1: {
            // Random color; random button text; FRK indicator forced; 3-battery layout
            randomizer = rosc_random32();
            scenario.bb_color      = randomizer % BB_NUM_COLORS;
            randomizer = rosc_random32();
            scenario.bb_text_idx   = randomizer % BUTTONTEXT_COUNT;
            scenario.indicator_idx = INDICATOR_IDX_FRK;  // always FRK

            randomizer = rosc_random32();
            if (randomizer % 2 == 0) {
                scenario.batt1 = BATT_TWO_AA; scenario.batt2 = BATT_ONE_D;
            } else {
                scenario.batt1 = BATT_ONE_D;  scenario.batt2 = BATT_TWO_AA;
            }
            break;
        }

        case 2: {
            // Red button; "Hold" text; random indicator; wide battery variety
            scenario.bb_color    = BB_COLOR_RED;
            scenario.bb_text_idx = 4;  // "Hold"

            randomizer = rosc_random32();
            scenario.indicator_idx = randomizer % INDICATOR_COUNT;

            randomizer = rosc_random32();
            switch (randomizer % 15) {
            case 0:  scenario.batt1 = BATT_ONE_AA; scenario.batt2 = BATT_EMPTY;  break;
            case 1:  scenario.batt1 = BATT_TWO_AA; scenario.batt2 = BATT_EMPTY;  break;
            case 2:  scenario.batt1 = BATT_EMPTY;  scenario.batt2 = BATT_ONE_D;  break;
            case 3:  scenario.batt1 = BATT_EMPTY;  scenario.batt2 = BATT_ONE_AA; break;
            case 4:  scenario.batt1 = BATT_EMPTY;  scenario.batt2 = BATT_TWO_AA; break;
            case 5:  scenario.batt1 = BATT_ONE_D;  scenario.batt2 = BATT_EMPTY;  break;
            case 6:  scenario.batt1 = BATT_ONE_AA; scenario.batt2 = BATT_ONE_AA; break;
            case 7:  scenario.batt1 = BATT_TWO_AA; scenario.batt2 = BATT_TWO_AA; break;
            case 8:  scenario.batt1 = BATT_ONE_D;  scenario.batt2 = BATT_ONE_D;  break;
            case 9:  scenario.batt1 = BATT_ONE_AA; scenario.batt2 = BATT_TWO_AA; break;
            case 10: scenario.batt1 = BATT_TWO_AA; scenario.batt2 = BATT_ONE_AA; break;
            case 11: scenario.batt1 = BATT_ONE_AA; scenario.batt2 = BATT_ONE_D;  break;
            case 12: scenario.batt1 = BATT_ONE_D;  scenario.batt2 = BATT_ONE_AA; break;
            case 13: scenario.batt1 = BATT_TWO_AA; scenario.batt2 = BATT_ONE_D;  break;
            case 14: scenario.batt1 = BATT_ONE_D;  scenario.batt2 = BATT_TWO_AA; break;
            }
            break;
        }
        } // end switch(randomizer % 3)

    } else {
        // ----------------------------------------------------------------
        // HOLD branch
        // TODO: flesh out full hold sub-cases (color, text, indicator,
        //       batteries) to match KTANE manual hold rules.
        //       For now: blue button, random text, random indicator,
        //       random batteries to keep the game playable.
        // ----------------------------------------------------------------
        scenario.bb_color = BB_COLOR_BLUE;

        randomizer = rosc_random32();
        scenario.bb_text_idx   = randomizer % BUTTONTEXT_COUNT;
        scenario.indicator_idx = (randomizer >> 8) % INDICATOR_COUNT;

        randomizer = rosc_random32();
        switch (randomizer % 4) {
        case 0: scenario.batt1 = BATT_ONE_AA; scenario.batt2 = BATT_EMPTY;  break;
        case 1: scenario.batt1 = BATT_EMPTY;  scenario.batt2 = BATT_ONE_AA; break;
        case 2: scenario.batt1 = BATT_ONE_AA; scenario.batt2 = BATT_ONE_D;  break;
        case 3: scenario.batt1 = BATT_ONE_D;  scenario.batt2 = BATT_ONE_AA; break;
        }
    }

    // ----------------------------------------------------------------
    // Update init_payload so all modules receive correct battery/indicator
    // counts when MSG_INIT is broadcast
    // ----------------------------------------------------------------
    battery_slots_to_counts(scenario.batt1, scenario.batt2,
                             &g->init_payload.battery_aa,
                             &g->init_payload.battery_d);

    g->init_payload.indicators =
        (scenario.indicator_idx == INDICATOR_IDX_FRK) ? IND_FRK : 0;

    // ----------------------------------------------------------------
    // Render battery and indicator displays
    // ----------------------------------------------------------------
    // Battery screen 1 — draw function clears fb internally; just flush after
    tca_select(tca_ch_batt1);
    switch (scenario.batt1) {
    case BATT_ONE_AA: ssd1306_draw_battery_one_aa(&disp_batt1); break;
    case BATT_TWO_AA: ssd1306_draw_battery_two_aa(&disp_batt1); break;
    case BATT_ONE_D:  ssd1306_draw_battery_one_d(&disp_batt1);  break;
    default: ssd1306_fill(&disp_batt1, 0); break;  // BATT_EMPTY — blank screen
    }
    ssd1306_flush(&disp_batt1);

    // Battery screen 2
    tca_select(tca_ch_batt2);
    switch (scenario.batt2) {
    case BATT_ONE_AA: ssd1306_draw_battery_one_aa(&disp_batt2); break;
    case BATT_TWO_AA: ssd1306_draw_battery_two_aa(&disp_batt2); break;
    case BATT_ONE_D:  ssd1306_draw_battery_one_d(&disp_batt2);  break;
    default: ssd1306_fill(&disp_batt2, 0); break;  // BATT_EMPTY — blank screen
    }
    ssd1306_flush(&disp_batt2);

    // Indicator screen
    tca_select(tca_ch_indicator);
    ssd1306_fill(&disp_indicator, 0);
    ssd1306_draw_str_aligned(&disp_indicator, 0, SSD1306_WIDTH,
                              8, INDICATORS[scenario.indicator_idx],
                              1, 2, ALIGN_CENTER);
    ssd1306_flush(&disp_indicator);

    tca_select_none();
}

// ---------------------------------------------------------------------------
// Big Button (local — runs on this Pico, no UART needed)
// ---------------------------------------------------------------------------
static uint8_t  bb_color;
static uint8_t  bb_text_idx;  // index into BUTTONTEXT[]
static bool     bb_held;
static uint32_t bb_press_start_ms;
static bool     bb_solved;

static void big_button_init(void) {
    gpio_init(BIG_BUTTON_PIN);
    gpio_set_dir(BIG_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BIG_BUTTON_PIN);

    gpio_init(BUTTON_GND_PIN);
    gpio_set_dir(BUTTON_GND_PIN, GPIO_OUT);
    gpio_put(BUTTON_GND_PIN, 0);

    // Color and text come from the scenario — set by scenario_randomize()
    bb_color    = scenario.bb_color;
    bb_text_idx = scenario.bb_text_idx;
    bb_solved   = false;
    bb_held     = false;

    // TODO: set WS2812 strip color to bb_color
    display_set_big_button_label(BUTTONTEXT[bb_text_idx]);
}

// Returns true if the Big Button defuse rule says release immediately
// (as opposed to hold and release on a specific timer digit).
// Rules from the KTANE manual:
//   - Blue + "Abort"           → hold
//   - More than 1 battery + "Detonate" → press/release immediately
//   - FRK indicator lit       → press/release immediately
//   - Yellow                  → hold
//   - Red + "Hold"             → press/release immediately
//   - otherwise               → hold
static bool bb_should_release_immediately(const GameState *g) {
    bool has_frk = (g->init_payload.indicators & IND_FRK) != 0;
    uint8_t total_batteries = g->init_payload.battery_aa + g->init_payload.battery_d;
    const char *text = BUTTONTEXT[bb_text_idx];

    // "Blue" + "Abort" → hold (not immediate)
    if (bb_color == BB_COLOR_BLUE && strcmp(text, "Abort") == 0) return false;

    // >1 battery + "Detonate" → immediate
    if (total_batteries > 1 && strcmp(text, "Detonate") == 0) return true;

    // FRK indicator → immediate
    if (has_frk) return true;

    // Yellow → hold
    if (bb_color == BB_COLOR_YELLOW) return false;

    // Red + "Hold" → immediate
    if (bb_color == BB_COLOR_RED && strcmp(text, "Hold") == 0) return true;

    // Default → hold
    return false;
}

// Returns the digit the player must release on when holding (1, 4, or 5).
static uint8_t bb_hold_release_digit(void) {
    switch (bb_color) {
    case BB_COLOR_BLUE:   return 4;
    case BB_COLOR_YELLOW: return 5;
    default:              return 1;
    }
}

static void big_button_poll(GameState *g) {
    if (bb_solved) return;
    bool pressed = !gpio_get(BIG_BUTTON_PIN); // active low
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (pressed && !bb_held) {
        bb_held = true;
        bb_press_start_ms = now;
    }

    if (!pressed && bb_held) {
        bb_held = false;
        uint32_t held_ms = now - bb_press_start_ms;

        if (bb_should_release_immediately(g)) {
            // Correct — immediate release
            bb_solved = true;
            g->big_button_solved = true;
            game_on_module_solved(g, 0); // slot 0 = big button
        } else {
            // Must have held and released on the right timer digit
            uint32_t remaining = game_remaining_ms(g);
            uint32_t seconds   = (remaining / 1000) % 60;
            uint8_t  ones_digit = seconds % 10;
            uint8_t  target    = bb_hold_release_digit();
            if (ones_digit == target) {
                bb_solved = true;
                g->big_button_solved = true;
                game_on_module_solved(g, 0);
            } else {
                game_on_strike(g, 0);
            }
        }
        (void)held_ms;
    }
}

// ---------------------------------------------------------------------------
// Inter-Pico bus service (called every main-loop iteration)
//
// I2C slaves cannot start a transfer, so nothing arrives unless we ask for it.
// Two mechanisms cover that:
//
//   Fast path — a module with news pulls the shared ATTN line low. We see it
//   within one loop iteration and sweep every active module, draining each
//   until it reports MSG_NONE.
//
//   Slow path — one module gets polled every BUS_KEEPALIVE_MS regardless of
//   ATTN, round-robin. This is the backstop: a broken ATTN wire costs latency
//   instead of silencing a module outright, and it doubles as liveness
//   detection for a module that has crashed or come unplugged.
// ---------------------------------------------------------------------------

// How often to poll one module when ATTN is idle.
#define BUS_KEEPALIVE_MS    200
// Cap on frames drained from a single module in one sweep, so a module stuck
// asserting ATTN cannot spin the loop forever.
#define BUS_DRAIN_LIMIT     4

static void bus_handle_message(GameState *g, const BusMessage *msg) {
    if (msg->src >= MODULE_ID_COUNT) return;

    g->modules[msg->src].last_ping_ms = to_ms_since_boot(get_absolute_time());

    switch (msg->type) {
    case MSG_PONG:
    case MSG_STATUS:
        // last_ping_ms above is the whole point of these
        break;

    case MSG_READY:
        game_on_module_ready(g, msg->src);
        break;

    case MSG_SOLVED:
        game_on_module_solved(g, msg->src);
        break;

    case MSG_STRIKE:
        game_on_strike(g, msg->src);
        display_update_strikes(g->strikes);
        break;

    default:
        break;
    }
}

// Read from one module until it has nothing left to say.
static void bus_drain_module(GameState *g, uint8_t id) {
    BusMessage msg;
    for (int i = 0; i < BUS_DRAIN_LIMIT; i++) {
        if (!bus_poll_module(id, &msg)) return;   // empty, absent, or corrupt
        bus_handle_message(g, &msg);
    }
}

static void bus_service(GameState *g) {
    static uint8_t  next_keepalive = MODULE_ID_FIRST;
    static uint32_t last_keepalive_ms = 0;

    if (bus_attn_asserted()) {
        // Someone has news but the line does not say who — sweep everyone.
        for (uint8_t id = MODULE_ID_FIRST; id <= MODULE_ID_LAST; id++) {
            if (g->modules[id].enabled) bus_drain_module(g, id);
        }
        // Still asserted? Then it is a module we are not talking to this level
        // (stale state, or a Pico that rebooted mid-game). Drain it too, or
        // ATTN stays low forever and starves the keepalive below.
        if (bus_attn_asserted()) {
            for (uint8_t id = MODULE_ID_FIRST; id <= MODULE_ID_LAST; id++) {
                if (!g->modules[id].enabled) bus_drain_module(g, id);
            }
        }
        return;
    }

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_keepalive_ms < BUS_KEEPALIVE_MS) return;
    last_keepalive_ms = now;

    // Round-robin one enabled module per keepalive tick.
    for (uint8_t tries = 0; tries < MODULE_ID_LAST; tries++) {
        uint8_t id = next_keepalive;
        next_keepalive = (id >= MODULE_ID_LAST) ? MODULE_ID_FIRST : (uint8_t)(id + 1);
        if (g->modules[id].enabled) {
            bus_drain_module(g, id);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Level select (simple: button cycles through levels on startup)
// ---------------------------------------------------------------------------
static void run_menu(GameState *g) {
    // Placeholder: press the big button to advance level, hold to start
    // TODO: proper level-select UI on the displays

    // 1. Randomise which physical slots carry Serial, Indicator, Batteries
    displays_randomize_channels();

    // 2. Load level config (sets serial, repeat_count; zeroes batteries/indicators)
    game_load_level(g, g->level);

    // 3. Randomise batteries, indicator, and Big Button setup; renders those displays
    scenario_randomize(g);

    // 4. Apply scenario to Big Button hardware
    big_button_init();

    // 5. Render serial number display
    display_show_serial((const char *)g->init_payload.serial);

    // 6. Broadcast MSG_INIT to all modules with the fully populated init_payload
    game_send_init(g);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    stdio_init_all();

    // I2C for displays and TCA9548A mux
    i2c_init(I2C_BUS, I2C_FREQ_HZ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    sleep_ms(50); // let displays power up

    // Set default channel assignments before first displays_init()
    // (run_menu will randomise them again each game start)
    tca_ch_serial    = 3;
    tca_ch_indicator = 7;
    tca_ch_batt1     = 1;
    tca_ch_batt2     = 2;

    // Init all displays via TCA mux
    displays_init();

    // Inter-Pico bus — central is the I2C master on i2c1 (GP2/GP3) + ATTN GP6
    bus_master_init();

    // Load persistent save (level progress)
    save_load(&save);

    // Game state — start from the saved level
    game_state_init(&gs);
    gs.level = save.current_level;

    // -----------------------------------------------------------------------
    // TESTING OVERRIDE — forces the Big Button hardware test level.
    // Remove this line (or comment it out) before shipping.
    // -----------------------------------------------------------------------
    gs.level = LEVEL_TEST;
    // Note: big_button_init() is called inside run_menu() after scenario_randomize(),
    // so it is NOT called here directly.

    // Start at menu (randomises scenario, inits Big Button, sends MSG_INIT)
    run_menu(&gs);

    while (true) {
        // Collect anything the modules have queued on the bus
        bus_service(&gs);

        // Per-state logic
        switch (gs.state) {

        case STATE_MENU:
        case STATE_INITIALIZING:
            // Waiting for all modules to reply MSG_READY (collected in bus_service)
            break;

        case STATE_ACTIVE:
            game_tick(&gs);
            display_update_timer(gs.remaining_ms);
            big_button_poll(&gs);
            break;

        case STATE_WIN: {
            // One-time setup on first entry
            static bool win_saved    = false;
            static bool win_init     = false;
            static ScrollState win_scroll;

            if (!win_init) {
                win_init = true;
                win_saved = false;
                scroll_start(&win_scroll, "YOU WIN");
            }

            // Save progress once (advancing to next level)
            if (!win_saved) {
                if (gs.level >= save.highest_level_unlocked) {
                    save.highest_level_unlocked = gs.level + 1;
                }
                save.current_level = gs.level + 1;
                save_write(&save);
                win_saved = true;
            }

            // Scroll "YOU WIN" on the 7-seg; loop it until button pressed
            bool finished = scroll_tick(&win_scroll);
            if (finished) scroll_start(&win_scroll, "YOU WIN");  // loop

            // TODO: add LED victory animation here

            // Button press returns to menu
            if (!gpio_get(BIG_BUTTON_PIN)) {
                win_init = false;  // reset for next time
                ht16k33_clear(&disp_timer);
                game_reset(&gs);
                gs.level = save.current_level;
                run_menu(&gs);
            }
            break;
        }

        case STATE_LOSE: {
            // One-time setup on first entry
            static bool lose_saved = false;
            static bool lose_init  = false;
            static ScrollState lose_scroll;

            if (!lose_init) {
                lose_init  = true;
                lose_saved = false;
                scroll_start(&lose_scroll, "YOU LOSE");
            }

            // Save current level once so power-cycle resumes here
            if (!lose_saved) {
                save.current_level = gs.level;
                save_write(&save);
                lose_saved = true;
            }

            // Scroll "YOU LOSE" on the 7-seg; loop it until button pressed
            bool finished = scroll_tick(&lose_scroll);
            if (finished) scroll_start(&lose_scroll, "YOU LOSE");  // loop

            // TODO: add LED explosion animation here

            // Button press returns to menu (same level — no penalty)
            if (!gpio_get(BIG_BUTTON_PIN)) {
                lose_init = false;  // reset for next time
                ht16k33_clear(&disp_timer);
                game_reset(&gs);
                gs.level = save.current_level;
                run_menu(&gs);
            }
            break;
        }
        }

        sleep_ms(1); // ~1 kHz poll rate
    }
}
