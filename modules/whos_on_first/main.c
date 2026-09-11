#include "pico/stdlib.h"
#include "../../shared/module_base.h"
#include "../../shared/bus_protocol.h"

// ---------------------------------------------------------------------------
// Who's on First module
// TODO: add module-specific #includes and pin defines here
// ---------------------------------------------------------------------------

static ModuleCtx ctx;

// ---------------------------------------------------------------------------
// Module callbacks
// ---------------------------------------------------------------------------
void on_module_init(ModuleCtx *c) {
    // TODO: read game parameters from c->game, configure hardware for this level
    (void)c;
}

void on_module_start(ModuleCtx *c) {
    // TODO: show puzzle on display, enable input
    (void)c;
}

void on_module_stop(ModuleCtx *c, uint8_t reason) {
    // TODO: show win/lose state
    (void)c; (void)reason;
}

void on_module_reset(ModuleCtx *c) {
    // TODO: clear display, reset hardware to idle
    (void)c;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    stdio_init_all();
    module_init(&ctx, MODULE_ID_WHOS_ON_FIRST);

    while (true) {
        module_poll(&ctx);

        if (ctx.lifecycle == MOD_ACTIVE) {
            // TODO: poll buttons / inputs and call module_send_solved() or
            // module_send_strike() when the player acts
        }

        sleep_ms(10);
    }
}
