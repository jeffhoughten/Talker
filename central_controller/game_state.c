#include "game_state.h"
#include "../shared/module_base.h"
#include <string.h>
#include <stdio.h>

static const char SERIAL_CHARS[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

void game_state_init(GameState *gs) {
    memset(gs, 0, sizeof(*gs));
    gs->state = STATE_MENU;
    gs->level = 0;
}

void game_generate_serial(char *out, uint8_t level_seed) {
    uint32_t r = rosc_random32() ^ ((uint32_t)level_seed << 16);
    for (int i = 0; i < 6; i++) {
        r = r * 6364136223846793005ULL + 1442695040888963407ULL;
        out[i] = SERIAL_CHARS[r >> 27];
    }
    out[6] = '\0';
}

void game_load_level(GameState *gs, uint8_t level) {
    if (level >= MAX_LEVELS) level = MAX_LEVELS - 1;
    gs->level   = level;
    gs->strikes = 0;
    gs->modules_ready   = 0;
    gs->modules_solved  = 0;
    gs->big_button_solved = false;

    const LevelConfig *cfg = &LEVELS[level];

    // Determine which remote modules are active
    gs->modules_required = 0;
    memset(gs->modules, 0, sizeof(gs->modules));

    for (uint8_t id = MODULE_ID_WIRES; id <= MODULE_ID_KNOB; id++) {
        uint16_t bit = (1 << id);
        gs->modules[id].enabled = (cfg->active_modules & bit) != 0;
        if (gs->modules[id].enabled) gs->modules_required++;
    }

    // Big Button (local — count it separately)
    bool bb_active = (cfg->active_modules & MODULE_BIT_BIG_BUTTON) != 0;
    gs->modules[0].enabled = bb_active;
    if (bb_active) gs->modules_required++;

    // Build init payload — serial and repeat count only.
    // Batteries and indicators are set by scenario_randomize() in main.c
    // after this function returns, so they are zeroed here as a baseline.
    GameInitPayload *p = &gs->init_payload;
    p->level        = level;
    p->repeat_count = cfg->repeat_count;
    p->battery_aa   = 0;
    p->battery_d    = 0;
    p->indicators   = 0;

    // Generate random serial
    game_generate_serial((char *)p->serial, level);
}

// I2C has no broadcast we can rely on across 10 slaves, so every "broadcast"
// is a loop of unicasts to the modules active this level. Keeping the mask in
// one helper means the send sites below stay one-liners.
static inline uint16_t active_mask(const GameState *gs) {
    return LEVELS[gs->level].active_modules;
}

void game_send_init(GameState *gs) {
    gs->state = STATE_INITIALIZING;
    gs->modules_ready = 0;

    BusMessage msg;
    msg_build(&msg, MODULE_ID_CENTRAL, MSG_INIT,
              (const uint8_t *)&gs->init_payload, sizeof(GameInitPayload));
    bus_broadcast(&msg, active_mask(gs));
}

void game_on_module_ready(GameState *gs, uint8_t module_id) {
    if (module_id >= MODULE_ID_COUNT) return;
    if (!gs->modules[module_id].ready) {
        gs->modules[module_id].ready = true;
        gs->modules_ready++;
    }
    if (gs->state == STATE_INITIALIZING && game_all_modules_ready(gs)) {
        game_start(gs);
    }
}

void game_on_module_solved(GameState *gs, uint8_t module_id) {
    if (module_id >= MODULE_ID_COUNT) return;
    if (!gs->modules[module_id].solved) {
        gs->modules[module_id].solved = true;
        gs->modules_solved++;
    }
    if (game_all_modules_solved(gs)) {
        game_win(gs);
    }
}

void game_on_strike(GameState *gs, uint8_t module_id) {
    (void)module_id;
    gs->strikes++;
    if (gs->strikes >= LEVELS[gs->level].max_strikes) {
        game_lose(gs);
    }
}

void game_tick(GameState *gs) {
    if (gs->state != STATE_ACTIVE) return;

    uint32_t elapsed = (uint32_t)(to_ms_since_boot(get_absolute_time()) -
                                   to_ms_since_boot(gs->start_time));
    uint32_t total   = LEVELS[gs->level].time_ms;

    if (elapsed >= total) {
        gs->remaining_ms = 0;
        game_lose(gs);
    } else {
        gs->remaining_ms = total - elapsed;
    }
}

void game_start(GameState *gs) {
    gs->state = STATE_ACTIVE;
    gs->start_time = get_absolute_time();
    gs->remaining_ms = LEVELS[gs->level].time_ms;

    BusMessage msg;
    msg_build_simple(&msg, MODULE_ID_CENTRAL, MSG_START);
    bus_broadcast(&msg, active_mask(gs));
}

void game_win(GameState *gs) {
    gs->state = STATE_WIN;
    uint8_t reason = STOP_WIN;
    BusMessage msg;
    msg_build(&msg, MODULE_ID_CENTRAL, MSG_STOP, &reason, 1);
    bus_broadcast(&msg, active_mask(gs));
}

void game_lose(GameState *gs) {
    gs->state = STATE_LOSE;
    uint8_t reason = STOP_LOSE;
    BusMessage msg;
    msg_build(&msg, MODULE_ID_CENTRAL, MSG_STOP, &reason, 1);
    bus_broadcast(&msg, active_mask(gs));
}

void game_reset(GameState *gs) {
    gs->state = STATE_MENU;
    BusMessage msg;
    msg_build_simple(&msg, MODULE_ID_CENTRAL, MSG_RESET);
    bus_broadcast(&msg, active_mask(gs));
}

bool game_all_modules_ready(const GameState *gs) {
    for (uint8_t id = 0; id < MODULE_ID_COUNT; id++) {
        if (gs->modules[id].enabled && !gs->modules[id].ready) return false;
    }
    return true;
}

bool game_all_modules_solved(const GameState *gs) {
    // Check remote modules
    for (uint8_t id = MODULE_ID_WIRES; id <= MODULE_ID_KNOB; id++) {
        if (gs->modules[id].enabled && !gs->modules[id].solved) return false;
    }
    // Check Big Button (local)
    if (gs->modules[0].enabled && !gs->big_button_solved) return false;
    return true;
}

uint32_t game_remaining_ms(const GameState *gs) {
    return gs->remaining_ms;
}
