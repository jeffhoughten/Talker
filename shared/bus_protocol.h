#pragma once

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ===========================================================================
// KTANE inter-Pico bus — I2C multi-drop
//
// Topology (replaces the old UART daisy chain):
//
//     Central (MASTER)                                       3V3
//        i2c1                                                 |
//     GP2 SDA  o---+-------+-------+-------+-- ... ---+      [4k7] x2
//     GP3 SCL  o---|---+---|---+---|---+---|-- ... ---|--+     |
//     GP6 ATTN o---|---|---|---|---|---|---|----------|--|---[4k7]
//                  |   |   |   |   |   |   |          |  |
//                 Mod 1   Mod 2   Mod 3        ...    Mod 10
//                 0x11    0x12    0x13                0x1A
//
// Every module is an I2C *slave* at address BUS_ADDR_BASE + module_id.
// Because slaves cannot initiate a transfer, a module that has something to
// report (strike, solved, ready) does two things:
//   1. Queues the frame in its outbox.
//   2. Pulls the shared ATTN line LOW (open-drain).
// The central sees ATTN low and sweeps the bus to collect the frames.
// ATTN is a "someone has news" flag only — it does not identify who.
//
// A round-robin keepalive poll also runs regardless of ATTN, so a broken
// ATTN wire degrades responsiveness instead of silencing a module.
// ===========================================================================

// ---------------------------------------------------------------------------
// Indicator bitmask flags (uint16_t — 11 indicators need more than 8 bits)
// Single source of truth; included by every Pico via this header.
// ---------------------------------------------------------------------------
#define IND_SND ((uint16_t)0x001)
#define IND_CLR ((uint16_t)0x002)
#define IND_IND ((uint16_t)0x004)
#define IND_FRQ ((uint16_t)0x008)
#define IND_SIG ((uint16_t)0x010)
#define IND_NSA ((uint16_t)0x020)
#define IND_MSA ((uint16_t)0x040)
#define IND_TRN ((uint16_t)0x080)
#define IND_BOB ((uint16_t)0x100)
#define IND_CAR ((uint16_t)0x200)
#define IND_FRK ((uint16_t)0x400)

// ---------------------------------------------------------------------------
// Module IDs
// ---------------------------------------------------------------------------
#define MODULE_ID_CENTRAL       0x00
#define MODULE_ID_WIRES         0x01
// Big Button is handled by the central controller (no separate Pico)
#define MODULE_ID_KEYPAD        0x02
#define MODULE_ID_SIMON         0x03
#define MODULE_ID_WHOS_ON_FIRST 0x04
#define MODULE_ID_MEMORY        0x05
#define MODULE_ID_SWITCHES      0x06
#define MODULE_ID_MAZE          0x07
#define MODULE_ID_PASSWORD      0x08
#define MODULE_ID_VENTING_GAS   0x09
#define MODULE_ID_KNOB          0x0A

#define MODULE_ID_FIRST         MODULE_ID_WIRES
#define MODULE_ID_LAST          MODULE_ID_KNOB
#define MODULE_ID_COUNT         12   // sizing for per-module arrays

// ---------------------------------------------------------------------------
// Message types
// Central → Module
// ---------------------------------------------------------------------------
#define MSG_NONE        0x00  // "outbox empty" — never a real message
#define MSG_PING        0x01  // Heartbeat; expects MSG_PONG
#define MSG_INIT        0x02  // Game params (level, serial, batteries, indicators)
#define MSG_START       0x03  // Begin active gameplay
#define MSG_STOP        0x04  // Game over — payload: STOP_WIN or STOP_LOSE
#define MSG_RESET       0x05  // Return to idle
#define MSG_LOCK        0x06  // Disable this module (not active this level)
#define MSG_UNLOCK      0x07  // Enable this module

// Module → Central
#define MSG_PONG        0x11  // Response to ping
#define MSG_READY       0x12  // Module initialised and waiting for MSG_START
#define MSG_SOLVED      0x13  // Module defused successfully
#define MSG_STRIKE      0x14  // Player made a mistake; central tracks strike count
#define MSG_STATUS      0x15  // Periodic heartbeat from module (optional)
#define MSG_ACK         0x16  // Generic acknowledge

// Stop reason values (payload byte 0 of MSG_STOP)
#define STOP_WIN        0x01
#define STOP_LOSE       0x02
#define STOP_RESET      0x03

// ---------------------------------------------------------------------------
// Bus wiring / timing
// ---------------------------------------------------------------------------
#define BUS_I2C             i2c1
#define BUS_SDA_PIN         2    // GP2, physical pin 4
#define BUS_SCL_PIN         3    // GP3, physical pin 5

// Shared open-drain attention line. Modules pull LOW; single pull-up at the
// central end (4k7 external — the internal ~50k is too weak for 10 drops).
#define BUS_ATTN_PIN        6    // GP6, physical pin 9

// 100 kHz, not 400 kHz. With 11 devices on a metre of harness the bus
// capacitance is high enough that 400 kHz gets marginal. Raise it only after
// you have scoped the SCL edges on the real wiring.
#define BUS_FREQ_HZ         100000

// Any single transaction that stalls longer than this is abandoned so one
// wedged module cannot freeze the game loop. A 20-byte frame at 100 kHz is
// ~2 ms on the wire, so this leaves roughly 4x headroom for clock stretching.
#define BUS_TIMEOUT_US      8000

// 7-bit slave address of a module = BUS_ADDR_BASE + module_id.
// Gives 0x11 (Wires) .. 0x1A (Knob). Deliberately clear of the central's own
// display bus (i2c0: TCA 0x70, HT16K33 0x71, SSD1306 0x3C) and of the
// I2C reserved ranges 0x00-0x07 and 0x78-0x7F.
#define BUS_ADDR_BASE       0x10
#define BUS_ADDR_OF(id)     ((uint8_t)(BUS_ADDR_BASE + (id)))

// ---------------------------------------------------------------------------
// Frame format — fixed 20 bytes, always. A fixed size keeps the slave ISR
// trivial: no length negotiation, the master always clocks exactly 20 bytes.
//
//   byte  0     SRC          module id of the sender
//   byte  1     TYPE         MSG_* (MSG_NONE = nothing to report)
//   byte  2     LEN          valid payload bytes, 0..16
//   bytes 3-18  PAYLOAD      zero-padded
//   byte  19    CRC8         Dallas/Maxim over bytes 0..18
// ---------------------------------------------------------------------------
#define PROTO_MAX_PAYLOAD   16
#define BUS_FRAME_SIZE      20

typedef struct {
    uint8_t src;
    uint8_t type;
    uint8_t payload_len;
    uint8_t payload[PROTO_MAX_PAYLOAD];
} BusMessage;

// Depth of the per-module outbox. A strike immediately followed by a solve
// must not be lost while the central is between sweeps.
#define BUS_OUTBOX_DEPTH    4
#define BUS_INBOX_DEPTH     4

// ---------------------------------------------------------------------------
// Frame encode / decode (shared by both ends)
// ---------------------------------------------------------------------------
void bus_frame_encode(uint8_t *frame20, const BusMessage *msg);
bool bus_frame_decode(const uint8_t *frame20, BusMessage *out);  // false on bad CRC

// CRC-8 Dallas/Maxim, polynomial 0x31
uint8_t proto_crc8(const uint8_t *data, size_t len);

// Build outgoing messages
void msg_build(BusMessage *out, uint8_t src, uint8_t type,
               const uint8_t *payload, uint8_t payload_len);
void msg_build_simple(BusMessage *out, uint8_t src, uint8_t type);

// ---------------------------------------------------------------------------
// MASTER side — central controller only
// ---------------------------------------------------------------------------

// Configure i2c1 as master on GP2/GP3 and ATTN as a pulled-up input.
void bus_master_init(void);

// Write one frame to one module. Returns false if the module did not ACK
// (unplugged, crashed, wrong address).
bool bus_send(uint8_t module_id, const BusMessage *msg);

// Write the same frame to every module set in module_mask (bit N = module N).
// There is no I2C broadcast we can rely on, so this is a loop of unicasts.
// Returns the number of modules that ACKed.
uint8_t bus_broadcast(const BusMessage *msg, uint16_t module_mask);

// Read one frame from one module. Returns true only if a real message came
// back (valid CRC and type != MSG_NONE). Call repeatedly to drain a module.
bool bus_poll_module(uint8_t module_id, BusMessage *out);

// True while at least one module is holding the ATTN line low.
bool bus_attn_asserted(void);

// ---------------------------------------------------------------------------
// SLAVE side — modules only
// ---------------------------------------------------------------------------

// Configure i2c1 as a slave at BUS_ADDR_OF(my_id) and ATTN as open-drain out.
void bus_slave_init(uint8_t my_id);

// Pop the next message the central sent us. Non-blocking.
bool bus_receive(BusMessage *out);

// Queue a message for the central to collect and assert ATTN.
// Returns false if the outbox is full.
bool bus_send_to_central(const BusMessage *msg);

// Re-sync the ATTN line with the real outbox state. Cheap; call every loop
// so a lost edge cannot leave the line stuck either way.
void bus_attn_refresh(void);
