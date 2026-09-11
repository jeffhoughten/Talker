#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "../../shared/module_base.h"
#include "../../shared/bus_protocol.h"

// ---------------------------------------------------------------------------
// Wires module — Raspberry Pi Pico
//
// 6 wire slots, each with a unique resistor so the color is identified by
// reading the ADC voltage divider on GP26 (ADC0).
//
// Hardware:
//   GP2  / GP3  — inter-Pico I2C bus, slave 0x11 (bus_protocol.h)
//   GP6         — shared ATTN line (bus_protocol.h)
//   GP10 / GP11 / GP12 — status RGB LED (module_base.h)
//   GP26 (ADC0) — wire sense (6 wires via resistor ladder or analog mux)
//   3V3  (pin 36) — one end of each wire
//   GND  (pin 38) — shared return
// ---------------------------------------------------------------------------

// ADC pin for wire sense
#define WIRE_ADC_PIN    26  // GP26, physical pin 31 — ADC0

// Number of wire slots
#define NUM_WIRES       6

// ---------------------------------------------------------------------------
// Wire color encoding via resistor values
// Tune these thresholds for your actual resistor values.
// ADC is 12-bit (0–4095) on RP2040; 3V3 reference.
// ---------------------------------------------------------------------------
typedef enum {
    WIRE_NONE   = 0,  // No wire inserted
    WIRE_RED    = 1,
    WIRE_YELLOW = 2,
    WIRE_BLUE   = 3,
    WIRE_WHITE  = 4,
    WIRE_BLACK  = 5,
} WireColor;

// Resistor ADC thresholds — adjust to match your chosen resistor values.
// Below each threshold = that color.  Above all = NONE.
static const uint16_t WIRE_ADC_THRESHOLDS[NUM_WIRES] = {
    500,   // RED    — smallest resistor → lowest voltage
    1000,  // YELLOW
    1600,  // BLUE
    2200,  // WHITE
    2900,  // BLACK
    3500,  // (last wire, adjust as needed)
};

// ---------------------------------------------------------------------------
// Wire state
// ---------------------------------------------------------------------------
static ModuleCtx ctx;
static WireColor wire_colors[NUM_WIRES];  // set during init
static uint8_t   correct_wire;           // index of the wire to cut

// ---------------------------------------------------------------------------
// ADC helpers
// ---------------------------------------------------------------------------
static uint16_t read_adc(void) {
    adc_select_input(0); // ADC0 = GP26
    return adc_read();
}

// Identify wire color from ADC reading (uses first connected wire on the bus)
static WireColor adc_to_color(uint16_t raw) {
    for (int i = 0; i < NUM_WIRES; i++) {
        if (raw < WIRE_ADC_THRESHOLDS[i]) return (WireColor)(i + 1);
    }
    return WIRE_NONE;
}

// Detect which wires are physically present by cycling a GPIO select line.
// If your hardware uses a separate sense pin per wire, replace this with
// individual gpio_get() calls on those pins.
static void scan_wires(WireColor out[NUM_WIRES]) {
    // Stub: set all slots to a sample color for now.
    // Replace with actual hardware scanning logic.
    for (int i = 0; i < NUM_WIRES; i++) {
        out[i] = WIRE_NONE;
    }
    // Example: read the analog mux / resistor ladder
    uint16_t raw = read_adc();
    out[0] = adc_to_color(raw);
}

// ---------------------------------------------------------------------------
// Rule engine — determines which wire to cut
// Rules from the KTANE manual (simplified; full rules TBD):
//   - If exactly one red wire and no yellow wires: cut the last wire
//   - If the last wire is white and serial ends in odd digit: cut last wire
//   - ... (expand as you implement)
// Returns the 0-based index of the correct wire, or 0 as a fallback.
// ---------------------------------------------------------------------------
static uint8_t determine_correct_wire(const WireColor colors[NUM_WIRES],
                                       const GameInitPayload *g) {
    // Count colors
    int counts[6] = {0};
    int last_present = 0;
    for (int i = 0; i < NUM_WIRES; i++) {
        if (colors[i] != WIRE_NONE) {
            counts[colors[i]]++;
            last_present = i;
        }
    }
    bool serial_odd = (g->serial[5] - '0') % 2 != 0;

    // Rule 1: 3 wires, no red → cut second wire
    if (counts[WIRE_RED] == 0) return 1;

    // Rule 2: last wire is white, serial odd → cut last wire
    if (colors[last_present] == WIRE_WHITE && serial_odd) return last_present;

    // Rule 3: more than 1 red wire → cut last red
    if (counts[WIRE_RED] > 1) {
        for (int i = NUM_WIRES - 1; i >= 0; i--) {
            if (colors[i] == WIRE_RED) return i;
        }
    }

    // Fallback
    return last_present;
}

// ---------------------------------------------------------------------------
// Module callbacks (required by module_base.h)
// ---------------------------------------------------------------------------
void on_module_init(ModuleCtx *c) {
    scan_wires(wire_colors);
    correct_wire = determine_correct_wire(wire_colors, &c->game);
}

void on_module_start(ModuleCtx *c) {
    (void)c;
    // Display is implicit — wires are physical objects; nothing to render
}

void on_module_stop(ModuleCtx *c, uint8_t reason) {
    (void)c; (void)reason;
}

void on_module_reset(ModuleCtx *c) {
    (void)c;
    scan_wires(wire_colors);
    correct_wire = determine_correct_wire(wire_colors, &c->game);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    stdio_init_all();
    adc_init();
    adc_gpio_init(WIRE_ADC_PIN);

    module_init(&ctx, MODULE_ID_WIRES);

    // Track previous wire presence to detect "cutting" (wire removed)
    WireColor prev[NUM_WIRES] = {0};
    scan_wires(prev);

    while (true) {
        module_poll(&ctx);

        if (ctx.lifecycle == MOD_ACTIVE) {
            WireColor current[NUM_WIRES];
            scan_wires(current);

            for (int i = 0; i < NUM_WIRES; i++) {
                // Wire was present before, now absent = wire cut
                if (prev[i] != WIRE_NONE && current[i] == WIRE_NONE) {
                    if (i == correct_wire) {
                        module_send_solved(&ctx);
                    } else {
                        module_send_strike(&ctx);
                        // Rescan correct wire — rule engine may need to re-run
                        // on remaining wires per the manual's "cut" rules.
                        scan_wires(wire_colors);
                        correct_wire = determine_correct_wire(wire_colors, &ctx.game);
                    }
                }
                prev[i] = current[i];
            }
        }

        sleep_ms(50); // 20 Hz scan rate is fine for physical wire cuts
    }
}
