#pragma once

#include "pico/stdlib.h"
#include "../shared/bus_protocol.h"

// ---------------------------------------------------------------------------
// Level timing & difficulty — edit these to tune gameplay
// ---------------------------------------------------------------------------

#define MAX_LEVELS 6

// Index of the hardware test level — Big Button only, 3-minute timer.
// Set gs.level = LEVEL_TEST in main.c to force it; remove when done testing.
#define LEVEL_TEST 5

// Which front-panel modules are active at each level.
// Bit positions match MODULE_ID_* in bus_protocol.h.
// Big Button (handled by central) is always bit 0 of this mask when active.
#define MODULE_BIT_BIG_BUTTON   (1 << 0)
#define MODULE_BIT_WIRES        (1 << MODULE_ID_WIRES)
#define MODULE_BIT_KEYPAD       (1 << MODULE_ID_KEYPAD)
#define MODULE_BIT_SIMON        (1 << MODULE_ID_SIMON)
#define MODULE_BIT_WHOS_ON_FIRST (1 << MODULE_ID_WHOS_ON_FIRST)
#define MODULE_BIT_MEMORY       (1 << MODULE_ID_MEMORY)

// Back panel
#define MODULE_BIT_SWITCHES     (1 << MODULE_ID_SWITCHES)
#define MODULE_BIT_MAZE         (1 << MODULE_ID_MAZE)
#define MODULE_BIT_PASSWORD     (1 << MODULE_ID_PASSWORD)
#define MODULE_BIT_VENTING_GAS  (1 << MODULE_ID_VENTING_GAS)
#define MODULE_BIT_KNOB         (1 << MODULE_ID_KNOB)

#define FRONT_ALL  (MODULE_BIT_BIG_BUTTON | MODULE_BIT_WIRES | MODULE_BIT_KEYPAD | \
                    MODULE_BIT_SIMON | MODULE_BIT_WHOS_ON_FIRST | MODULE_BIT_MEMORY)
#define BACK_ALL   (MODULE_BIT_SWITCHES | MODULE_BIT_MAZE | MODULE_BIT_PASSWORD | \
                    MODULE_BIT_VENTING_GAS | MODULE_BIT_KNOB)

// IND_* defines live in shared/bus_protocol.h (included above).

typedef struct {
    // Time limit
    uint32_t time_ms;           // Countdown in milliseconds

    // Strike limit (game over when strikes == max_strikes)
    uint8_t  max_strikes;

    // Which modules are active this level (bitmask of MODULE_BIT_* values)
    uint16_t active_modules;

    // Repeat count for multi-round modules (Memory, Who's on First)
    uint8_t  repeat_count;

    // NOTE: batteries, indicators, and Big Button setup are fully randomised
    // each game start by scenario_randomize() in main.c.  They are no longer
    // stored in LevelConfig.
} LevelConfig;

// ---------------------------------------------------------------------------
// Level definitions
// Adjust time_ms to taste — these are deliberately forgiving for first runs.
// ---------------------------------------------------------------------------
static const LevelConfig LEVELS[MAX_LEVELS] = {
    // Level 1 — Tutorial: 3 front modules, generous time, 3 strikes
    { .time_ms = 300000, .max_strikes = 3, .repeat_count = 1,
      .active_modules = MODULE_BIT_BIG_BUTTON | MODULE_BIT_WIRES | MODULE_BIT_KEYPAD },

    // Level 2 — 4 front modules, 3 strikes
    { .time_ms = 270000, .max_strikes = 3, .repeat_count = 1,
      .active_modules = MODULE_BIT_BIG_BUTTON | MODULE_BIT_WIRES | MODULE_BIT_KEYPAD |
                        MODULE_BIT_SIMON },

    // Level 3 — All front modules, 2 strikes, repeats begin
    { .time_ms = 240000, .max_strikes = 2, .repeat_count = 2,
      .active_modules = FRONT_ALL },

    // Level 4 — Back panel unlocked, 2 strikes
    { .time_ms = 210000, .max_strikes = 2, .repeat_count = 3,
      .active_modules = FRONT_ALL | BACK_ALL },

    // Level 5 — Max difficulty: tight time, 1 strike
    { .time_ms = 180000, .max_strikes = 1, .repeat_count = 3,
      .active_modules = FRONT_ALL | BACK_ALL },

    // Level 5 / LEVEL_TEST — Hardware test: Big Button only, 3 min, 3 strikes
    { .time_ms = 180000, .max_strikes = 3, .repeat_count = 1,
      .active_modules = MODULE_BIT_BIG_BUTTON },
};
