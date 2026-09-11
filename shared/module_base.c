#include "module_base.h"
#include "hardware/gpio.h"
#include "hardware/regs/rosc.h"
#include "hardware/structs/rosc.h"
#include <string.h>

// ---------------------------------------------------------------------------
// ROSC-based random (ported from original ModuleState.cpp)
// ---------------------------------------------------------------------------
uint32_t rosc_random32(void) {
    uint32_t result = 0x811c9dc5u;
    volatile uint32_t *rnd_reg = (volatile uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);
    for (int i = 0; i < 32; i++) {
        result ^= (*rnd_reg & 1u);
        result *= 0x01000193u;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Status LED
// ---------------------------------------------------------------------------
static bool     strike_flash_active = false;
static uint32_t strike_flash_end_ms = 0;

void status_led_set(uint8_t r, uint8_t g, uint8_t b) {
    gpio_put(STATUS_LED_R_PIN, r > 0);
    gpio_put(STATUS_LED_G_PIN, g > 0);
    gpio_put(STATUS_LED_B_PIN, b > 0);
}

void status_led_idle(void)   { status_led_set(1, 1, 1); }  // white
void status_led_active(void) { status_led_set(0, 0, 1); }  // blue
void status_led_solved(void) { status_led_set(0, 1, 0); }  // green
void status_led_locked(void) { status_led_set(0, 0, 0); }  // off

void status_led_strike(void) {
    // Kick off a 500 ms red flash; resolves in module_poll()
    strike_flash_active = true;
    strike_flash_end_ms = to_ms_since_boot(get_absolute_time()) + 500;
    status_led_set(1, 0, 0);
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void module_init(ModuleCtx *ctx, uint8_t my_id) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->my_id     = my_id;
    ctx->lifecycle = MOD_IDLE;

    // Status LED pins
    gpio_init(STATUS_LED_R_PIN); gpio_set_dir(STATUS_LED_R_PIN, GPIO_OUT);
    gpio_init(STATUS_LED_G_PIN); gpio_set_dir(STATUS_LED_G_PIN, GPIO_OUT);
    gpio_init(STATUS_LED_B_PIN); gpio_set_dir(STATUS_LED_B_PIN, GPIO_OUT);
    status_led_idle();

    // Inter-Pico bus — we are an I2C slave at BUS_ADDR_OF(my_id)
    bus_slave_init(my_id);
}

// ---------------------------------------------------------------------------
// Outgoing helpers
// ---------------------------------------------------------------------------
void module_send_ready(ModuleCtx *ctx) {
    BusMessage m;
    msg_build_simple(&m, ctx->my_id, MSG_READY);
    bus_send_to_central(&m);
}

void module_send_solved(ModuleCtx *ctx) {
    ctx->lifecycle = MOD_SOLVED;
    status_led_solved();
    BusMessage m;
    msg_build_simple(&m, ctx->my_id, MSG_SOLVED);
    bus_send_to_central(&m);
}

void module_send_strike(ModuleCtx *ctx) {
    status_led_strike();
    BusMessage m;
    msg_build_simple(&m, ctx->my_id, MSG_STRIKE);
    bus_send_to_central(&m);
}

// ---------------------------------------------------------------------------
// Poll — call once per main-loop iteration
// ---------------------------------------------------------------------------
bool module_poll(ModuleCtx *ctx) {
    bool processed = false;

    // Keep ATTN honest even if an edge was missed in the ISR.
    bus_attn_refresh();

    // Resolve strike flash if running
    if (strike_flash_active) {
        if (to_ms_since_boot(get_absolute_time()) >= strike_flash_end_ms) {
            strike_flash_active = false;
            // Restore appropriate LED for current lifecycle
            switch (ctx->lifecycle) {
            case MOD_ACTIVE:   status_led_active(); break;
            case MOD_SOLVED:   status_led_solved(); break;
            case MOD_LOCKED:   status_led_locked(); break;
            default:           status_led_idle();   break;
            }
        }
    }

    // Drain everything the central sent us since the last pass. The I2C slave
    // ISR has already queued these, so there is nothing to forward on.
    BusMessage msg;
    while (bus_receive(&msg)) {
        processed = true;

        switch (msg.type) {

        case MSG_PING: {
            BusMessage pong;
            msg_build_simple(&pong, ctx->my_id, MSG_PONG);
            bus_send_to_central(&pong);
            break;
        }

        case MSG_INIT:
            if (msg.payload_len >= sizeof(GameInitPayload)) {
                memcpy(&ctx->game, msg.payload, sizeof(GameInitPayload));
            }
            ctx->lifecycle = MOD_READY;
            status_led_idle();
            on_module_init(ctx);
            module_send_ready(ctx);
            break;

        case MSG_START:
            ctx->lifecycle = MOD_ACTIVE;
            status_led_active();
            on_module_start(ctx);
            break;

        case MSG_STOP: {
            uint8_t reason = (msg.payload_len > 0) ? msg.payload[0] : STOP_LOSE;
            if (reason == STOP_WIN) ctx->lifecycle = MOD_SOLVED;
            else                    ctx->lifecycle = MOD_IDLE;
            on_module_stop(ctx, reason);
            break;
        }

        case MSG_RESET:
            ctx->lifecycle = MOD_IDLE;
            status_led_idle();
            on_module_reset(ctx);
            break;

        case MSG_LOCK:
            ctx->lifecycle = MOD_LOCKED;
            status_led_locked();
            break;

        case MSG_UNLOCK:
            ctx->lifecycle = MOD_IDLE;
            status_led_idle();
            break;

        default:
            break;
        }
    }

    return processed;
}
