#pragma once

#include "pico/stdlib.h"
#include "level_config.h"
#include "../shared/bus_protocol.h"
#include "../shared/module_base.h"

// ---------------------------------------------------------------------------
// Game states
// ---------------------------------------------------------------------------
typedef enum {
    STATE_MENU,         // Level select / idle — showing level info on displays
    STATE_INITIALIZING, // Sent MSG_INIT, waiting for all modules to reply MSG_READY
    STATE_ACTIVE,       // Countdown running, modules accepting input
    STATE_WIN,          // All modules solved
    STATE_LOSE,         // Strikes exceeded or timer expired
} GameStateEnum;

// ---------------------------------------------------------------------------
// Per-module tracking (central's view of each remote module)
// ---------------------------------------------------------------------------
typedef struct {
    bool     enabled;   // Active this level
    bool     ready;     // Replied MSG_READY
    bool     solved;    // Replied MSG_SOLVED
    uint32_t last_ping_ms;
} ModuleStatus;

// ---------------------------------------------------------------------------
// Central game state
// ---------------------------------------------------------------------------
typedef struct {
    GameStateEnum    state;

    uint8_t          level;         // 0-based index into LEVELS[]
    uint8_t          strikes;
    uint8_t          modules_ready;
    uint8_t          modules_solved;
    uint8_t          modules_required; // count of enabled modules

    absolute_time_t  start_time;
    uint32_t         remaining_ms;  // updated each tick

    // Big Button state (handled locally, not over the bus)
    bool             big_button_solved;

    // Remote module status table indexed by MODULE_ID_*
    ModuleStatus     modules[MODULE_ID_COUNT]; // 0 = central/big-button, 1-10 = remote

    // Current game init payload (sent to all modules on MSG_INIT)
    GameInitPayload  init_payload;
} GameState;

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------
void  game_state_init(GameState *gs);

// Load level config into gs and prepare init_payload (serial, batteries, etc.)
void  game_load_level(GameState *gs, uint8_t level);

// Broadcast MSG_INIT to all active modules; transition to STATE_INITIALIZING
void  game_send_init(GameState *gs);

// Called by the bus handler when MSG_READY received from a module
void  game_on_module_ready(GameState *gs, uint8_t module_id);

// Called by the bus handler when MSG_SOLVED received
void  game_on_module_solved(GameState *gs, uint8_t module_id);

// Called by the bus handler when MSG_STRIKE received
void  game_on_strike(GameState *gs, uint8_t module_id);

// Call every main-loop iteration when STATE_ACTIVE — updates timer, checks end conditions
void  game_tick(GameState *gs);

// Transition helpers
void  game_start(GameState *gs);   // Broadcast MSG_START
void  game_win(GameState *gs);
void  game_lose(GameState *gs);
void  game_reset(GameState *gs);   // Broadcast MSG_RESET, return to menu

bool  game_all_modules_ready(const GameState *gs);
bool  game_all_modules_solved(const GameState *gs);
uint32_t game_remaining_ms(const GameState *gs);

// Returns a random 6-char serial string; caller provides char[7] buffer
void  game_generate_serial(char *out, uint8_t level_seed);
