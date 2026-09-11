#include "bus_protocol.h"
#include "hardware/gpio.h"
#include <string.h>

// ===========================================================================
// Shared: CRC, framing, message builders
// ===========================================================================

uint8_t proto_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

void bus_frame_encode(uint8_t *frame20, const BusMessage *msg) {
    memset(frame20, 0, BUS_FRAME_SIZE);
    uint8_t len = msg->payload_len;
    if (len > PROTO_MAX_PAYLOAD) len = PROTO_MAX_PAYLOAD;

    frame20[0] = msg->src;
    frame20[1] = msg->type;
    frame20[2] = len;
    memcpy(&frame20[3], msg->payload, len);
    frame20[BUS_FRAME_SIZE - 1] = proto_crc8(frame20, BUS_FRAME_SIZE - 1);
}

bool bus_frame_decode(const uint8_t *frame20, BusMessage *out) {
    if (proto_crc8(frame20, BUS_FRAME_SIZE - 1) != frame20[BUS_FRAME_SIZE - 1]) {
        return false;
    }
    uint8_t len = frame20[2];
    if (len > PROTO_MAX_PAYLOAD) return false;

    out->src         = frame20[0];
    out->type        = frame20[1];
    out->payload_len = len;
    memcpy(out->payload, &frame20[3], len);
    if (len < PROTO_MAX_PAYLOAD) {
        memset(&out->payload[len], 0, (size_t)(PROTO_MAX_PAYLOAD - len));
    }
    return true;
}

void msg_build(BusMessage *out, uint8_t src, uint8_t type,
               const uint8_t *payload, uint8_t payload_len) {
    memset(out, 0, sizeof(*out));
    out->src  = src;
    out->type = type;
    if (payload && payload_len) {
        if (payload_len > PROTO_MAX_PAYLOAD) payload_len = PROTO_MAX_PAYLOAD;
        out->payload_len = payload_len;
        memcpy(out->payload, payload, payload_len);
    }
}

void msg_build_simple(BusMessage *out, uint8_t src, uint8_t type) {
    msg_build(out, src, type, NULL, 0);
}

// ===========================================================================
// MASTER side — central controller
// ===========================================================================

void bus_master_init(void) {
    i2c_init(BUS_I2C, BUS_FREQ_HZ);
    gpio_set_function(BUS_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BUS_SCL_PIN, GPIO_FUNC_I2C);
    // Internal pull-ups are a weak backup only; fit 4k7 externals on the bus.
    gpio_pull_up(BUS_SDA_PIN);
    gpio_pull_up(BUS_SCL_PIN);

    // ATTN: input, pulled up. Modules only ever pull it down.
    gpio_init(BUS_ATTN_PIN);
    gpio_set_dir(BUS_ATTN_PIN, GPIO_IN);
    gpio_pull_up(BUS_ATTN_PIN);
}

bool bus_send(uint8_t module_id, const BusMessage *msg) {
    uint8_t frame[BUS_FRAME_SIZE];
    bus_frame_encode(frame, msg);

    int n = i2c_write_timeout_us(BUS_I2C, BUS_ADDR_OF(module_id),
                                 frame, BUS_FRAME_SIZE, false, BUS_TIMEOUT_US);
    return n == BUS_FRAME_SIZE;
}

uint8_t bus_broadcast(const BusMessage *msg, uint16_t module_mask) {
    uint8_t acked = 0;
    for (uint8_t id = MODULE_ID_FIRST; id <= MODULE_ID_LAST; id++) {
        if (!(module_mask & (1u << id))) continue;
        if (bus_send(id, msg)) acked++;
    }
    return acked;
}

bool bus_poll_module(uint8_t module_id, BusMessage *out) {
    uint8_t frame[BUS_FRAME_SIZE];

    int n = i2c_read_timeout_us(BUS_I2C, BUS_ADDR_OF(module_id),
                                frame, BUS_FRAME_SIZE, false, BUS_TIMEOUT_US);
    if (n != BUS_FRAME_SIZE) return false;          // absent or wedged
    if (!bus_frame_decode(frame, out))  return false; // corrupt
    if (out->type == MSG_NONE)          return false; // nothing to report

    // Trust the I2C address over the frame's own claim about who it is.
    out->src = module_id;
    return true;
}

bool bus_attn_asserted(void) {
    return !gpio_get(BUS_ATTN_PIN);   // active low
}

// ===========================================================================
// SLAVE side — modules
//
// Only compiled into module firmware. The central never calls bus_slave_init()
// so the handler below simply stays dormant there.
// ===========================================================================

#include "pico/i2c_slave.h"

static uint8_t my_module_id = 0;

// ---- Inbox: central -> us. ISR produces, main loop consumes. ----
static volatile BusMessage inbox[BUS_INBOX_DEPTH];
static volatile uint8_t    ib_head = 0;   // written by ISR
static volatile uint8_t    ib_tail = 0;   // written by main loop

// ---- Outbox: us -> central. Main loop produces, ISR consumes. ----
static volatile BusMessage outbox[BUS_OUTBOX_DEPTH];
static volatile uint8_t    ob_head = 0;   // written by main loop
static volatile uint8_t    ob_tail = 0;   // written by ISR

// ---- Receive assembly ----
static uint8_t rx_buf[BUS_FRAME_SIZE];
static uint8_t rx_len = 0;
static uint8_t rx_overflow = 0;

// ---- Transmit assembly ----
static uint8_t tx_frame[BUS_FRAME_SIZE];
static uint8_t tx_idx = 0;
static bool    tx_started = false;
static bool    tx_pending_pop = false;

static inline bool outbox_empty(void) { return ob_head == ob_tail; }

// Open-drain: asserted = drive the pin low, released = let it float high.
// We never drive it high, so several modules can hold it at once.
static void attn_sync(void) {
    if (outbox_empty()) gpio_set_dir(BUS_ATTN_PIN, GPIO_IN);   // release
    else                gpio_set_dir(BUS_ATTN_PIN, GPIO_OUT);  // pull low
}

static void bus_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {

    case I2C_SLAVE_RECEIVE: {
        uint8_t b = i2c_read_byte_raw(i2c);
        if (rx_len < BUS_FRAME_SIZE) rx_buf[rx_len++] = b;
        else                         rx_overflow = 1;   // master sent too much
        break;
    }

    case I2C_SLAVE_REQUEST: {
        if (!tx_started) {
            tx_started = true;
            tx_idx     = 0;
            if (!outbox_empty()) {
                BusMessage m;
                memcpy(&m, (const void *)&outbox[ob_tail], sizeof(m));
                bus_frame_encode(tx_frame, &m);
                tx_pending_pop = true;
            } else {
                BusMessage none;
                msg_build_simple(&none, my_module_id, MSG_NONE);
                bus_frame_encode(tx_frame, &none);
                tx_pending_pop = false;
            }
        }
        i2c_write_byte_raw(i2c, (tx_idx < BUS_FRAME_SIZE) ? tx_frame[tx_idx] : 0x00);
        if (tx_idx < BUS_FRAME_SIZE) tx_idx++;
        break;
    }

    case I2C_SLAVE_FINISH: {
        // Finished a read: only drop the queued message once the master has
        // actually clocked out the whole frame.
        if (tx_started) {
            if (tx_pending_pop && tx_idx >= BUS_FRAME_SIZE) {
                ob_tail = (uint8_t)((ob_tail + 1) % BUS_OUTBOX_DEPTH);
            }
            tx_started     = false;
            tx_pending_pop = false;
            tx_idx         = 0;
        }

        // Finished a write: validate and queue it for the main loop.
        if (!rx_overflow && rx_len == BUS_FRAME_SIZE) {
            BusMessage m;
            if (bus_frame_decode(rx_buf, &m)) {
                uint8_t next = (uint8_t)((ib_head + 1) % BUS_INBOX_DEPTH);
                if (next != ib_tail) {          // drop rather than overwrite
                    memcpy((void *)&inbox[ib_head], &m, sizeof(m));
                    ib_head = next;
                }
            }
        }
        rx_len      = 0;
        rx_overflow = 0;

        attn_sync();
        break;
    }
    }
}

void bus_slave_init(uint8_t my_id) {
    my_module_id = my_id;
    ib_head = ib_tail = 0;
    ob_head = ob_tail = 0;
    rx_len  = 0;
    rx_overflow = 0;
    tx_idx  = 0;
    tx_started = false;
    tx_pending_pop = false;

    gpio_set_function(BUS_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BUS_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BUS_SDA_PIN);
    gpio_pull_up(BUS_SCL_PIN);

    // ATTN as open-drain. Latch the output register at 0 once, then switch
    // direction to assert/release — the pin is never driven high.
    gpio_init(BUS_ATTN_PIN);
    gpio_put(BUS_ATTN_PIN, 0);
    gpio_set_dir(BUS_ATTN_PIN, GPIO_IN);   // released
    gpio_disable_pulls(BUS_ATTN_PIN);      // central provides the single pull-up

    i2c_init(BUS_I2C, BUS_FREQ_HZ);
    i2c_slave_init(BUS_I2C, BUS_ADDR_OF(my_id), &bus_slave_handler);
}

bool bus_receive(BusMessage *out) {
    if (ib_head == ib_tail) return false;
    memcpy(out, (const void *)&inbox[ib_tail], sizeof(*out));
    ib_tail = (uint8_t)((ib_tail + 1) % BUS_INBOX_DEPTH);
    return true;
}

bool bus_send_to_central(const BusMessage *msg) {
    uint8_t next = (uint8_t)((ob_head + 1) % BUS_OUTBOX_DEPTH);
    if (next == ob_tail) return false;     // full

    memcpy((void *)&outbox[ob_head], msg, sizeof(*msg));
    ob_head = next;
    attn_sync();
    return true;
}

void bus_attn_refresh(void) {
    attn_sync();
}
