# KTANE Physical Bomb — Full System Diagram

---

## 1. Power Distribution

```
                         ┌─────────────────────────────────────────────────┐
                         │  2S Li-ion Pack  7.4V / 6 000 mAh              │
                         │  (2× 18650 in series)                           │
                         └───────────────────┬─────────────────────────────┘
                                             │ 7.4V
                                    ┌────────┴────────┐
                               B+   │  TP5100 / BMS   │  USB-C charging in
                                    │  2S charger +   │◄──────────────────
                                    │  protection     │
                                    └────────┬────────┘
                                             │ 7.4V  BAT−
                                    ┌────────┴────────┐   ┌──────────────────┐
                                    │  MASTER SWITCH  │   │  GND BUS         │
                                    │  DPDT  ≥ 3A     │   │  (18 AWG copper) │
                                    └────────┬────────┘   │  ══════════════  │
                                             │             └────────▲─────────┘
                          ┌──────────────────┤                      │ all GNDs home here
                          │                  │                      │
                   ┌──────┴──────┐    ┌──────┴──────┐              │
                   │  XL4016     │    │  AMS1117-3.3 │              │
                   │  Buck conv. │    │  LDO  1A     │              │
                   │  7.4→5.00V  │    │  5V → 3.3V   │              │
                   └──────┬──────┘    └──────┬───────┘              │
                          │                  │                      │
                       5V RAIL           3.3V RAIL                 GND
                          │                  │
          ┌───────────────┼──────────────────┼───────────────────────────────────────┐
          │               │                  │                                       │
          │    ┌──────────┴───┐   ┌──────────┴──────────────────────────────────┐   │
          │    │  5V BUS      │   │  3.3V BUS                                   │   │
          │    │  (perfboard  │   │  All OLEDs, TCA9548A chips, HT16K33,        │   │
          │    │   or busbar) │   │  SK9822 VCC, RGB LEDs, 7-seg modules        │   │
          │    └──────┬───────┘   └─────────────────────────────────────────────┘   │
          │           │                                                              │
          │    ┌──────┴───────────────────────────────────────────────────────┐     │
          │    │  per-Pico: 1N5817 Schottky on VSYS pin (×11)                │     │
          │    └──────────────────────────────────────────────────────────────┘     │
          │                                                                          │
          │  WS2812 VCC ──── 5V BUS    SK9822 strips ──── 3.3V BUS                  │
          │  WS2812 GND ─────────────────────────────────────────────────────────── ┘
```

---

## 2. Inter-Pico I2C Bus

All eleven Picos share one 4-wire bus: SDA, SCL, ATTN, GND. Every module taps
it in parallel, so there is no ordering and no chain to break — pulling one
module out does not isolate the ones "after" it.

```
                              3V3
                               │
                   ┌───────────┼───────────┐
                 [4k7]       [4k7]       [4k7]      ← fit ONCE, at the central
                   │           │           │
 ┌─────────────┐   │           │           │
 │  CENTRAL    │   │           │           │
 │ CONTROLLER  │   │           │           │
 │   Pico 1    │   │           │           │
 │  (MASTER)   │   │           │           │
 │             │   │           │           │
 │  GP2  SDA ──┼───┴───┬───────┬───────┬───────┬───────┬───────┬─── … ──┐
 │  GP3  SCL ──┼───────┼─┬─────┼─┬─────┼─┬─────┼─┬─────┼─┬───── … ──┐  │
 │  GP6  ATTN ─┼───────┼─┼─┬───┼─┼─┬───┼─┼─┬───┼─┼─┬───┼─┼───── … ─┐│  │
 │  GND ───────┼───────┼─┼─┼─┬─┼─┼─┼─┬─┼─┼─┼─┬─┼─┼─┼─┬─┼─┼───── … ┐││  │
 └─────────────┘       │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │         ││││
                       ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼ ▼         ▼▼▼▼
 FRONT PANEL        ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐
                    │ WIRES │ │KEYPAD │ │ SIMON │ │ WHO'S │ │MEMORY │
                    │Pico 2 │ │Pico 3 │ │Pico 4 │ │Pico 5 │ │Pico 6 │
                    │ 0x11  │ │ 0x12  │ │ 0x13  │ │ 0x14  │ │ 0x15  │
                    └───────┘ └───────┘ └───────┘ └───────┘ └───────┘

 BACK PANEL         ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐
                    │SWITCH │ │ MAZE  │ │PASSWD │ │VENTING│ │ KNOB  │
                    │Pico 7 │ │Pico 8 │ │Pico 9 │ │Pico 10│ │Pico 11│
                    │ 0x16  │ │ 0x17  │ │ 0x18  │ │ 0x19  │ │ 0x1A  │
                    └───────┘ └───────┘ └───────┘ └───────┘ └───────┘

  Every module:  GP2 → SDA bus,  GP3 → SCL bus,  GP6 → ATTN bus,  GND → GND bus
  Slave address: 0x10 + module_id, set in firmware at compile time
```

### The three signals

| Wire | GPIO | Who drives it                     | Idle state |
|------|------|-----------------------------------|------------|
| SDA  | GP2  | master and slaves (open-drain)    | high       |
| SCL  | GP3  | master only                       | high       |
| ATTN | GP6  | slaves pull low only, never high  | high       |

**Pull-ups go on once, at the central.** Three 4.7 kΩ to 3V3. The temptation is
to fit a pair per module because each breakout board has footprints for them —
don't. Ten 4.7 kΩ pull-ups in parallel is 470 Ω, which the RP2040's open-drain
output cannot pull low cleanly, and the bus stops working.

### How a module reports a strike

I2C slaves cannot initiate a transfer. The sequence is:

```
  1. Player makes a mistake on the Keypad.
  2. Keypad queues MSG_STRIKE in its outbox and pulls ATTN low.
  3. Central's main loop sees ATTN low (within ~1 ms).
  4. Central reads 20 bytes from each active module in turn.
  5. Keypad's read returns the strike frame; every other module returns
     MSG_NONE. Keypad's outbox is now empty, so it releases ATTN.
  6. Central increments the strike counter and updates the strike display.
```

If ATTN is left unwired the central still polls one module every 200 ms, so the
game works with roughly a 1-2 s worst-case delay on a strike. That is the
fallback, not the plan — wire ATTN.

### Bus speed

100 kHz, set by `BUS_FREQ_HZ` in `shared/bus_protocol.h`. Eleven devices spread
over a metre of harness is a lot of bus capacitance, and 400 kHz gets marginal
at that length. A 20-byte frame at 100 kHz takes about 2 ms, which is far
faster than any human input the game has to catch. Raise it only after you have
put a scope on SCL and confirmed the rising edges are square.

---

## 3. Central Controller Detail

```
                    ┌────────────────────────────────────────────────────────┐
                    │                  CENTRAL CONTROLLER                    │
                    │                   Raspberry Pi Pico                    │
                    ├───────────┬────────────────────────────────────────────┤
                    │           │                                            │
  BUS SDA ──────────┤ GP2  SDA  │   ← i2c1, master. To all 10 modules + 4k7   │
  BUS SCL ──────────┤ GP3  SCL  │   ← i2c1, master. To all 10 modules + 4k7   │
                    │           │                                            │
  I2C0 SDA   ───────┤ GP4  SDA ─┼───┤SDA  ┌────────────────────────────────┐ │
  I2C0 SCL   ───────┤ GP5  SCL ─┼───┤SCL  │      TCA9548A  (0x70)          │ │
                    │           │   │     │  CH0 ──── HT16K33 7-seg (0x71) │ │
  BUS ATTN ─────────┤ GP6       │   │     │  CH1 ──── SSD1306 128×64(0x3C) │ │  battery *
   (pulled up 4k7)  │           │   │     │  CH2 ──── SSD1306 128×64(0x3C) │ │  battery *
                    │           │   │     │  CH3 ──── SSD1306 128×32(0x3C) │ │  serial *
  RGB LED R  ───────┤ GP10      │   │     │  CH4 ──── SSD1306 128×64(0x3C) │ │  strikes
  RGB LED G  ───────┤ GP11      │   │     │  CH5 ──── (unused)             │ │
  RGB LED B  ───────┤ GP12      │   │     │  CH6 ──── SSD1306 128×64(0x3C) │ │  big button label
                    │           │   │     │  CH7 ──── SSD1306 128×32(0x3C) │ │  indicator *
  WS2812 DATA ──────┤ GP15      │   │     └────────────────────────────────┘ │
                    │           │                 * randomised each game     │
  SK9822 CLK  ──────┤ GP18      │                                            │
  SK9822 DATA ──────┤ GP19      │   ┌──────────────────────────────────────┐ │
  Big Button ───────┤ GP20      │   │  WS2812 × 12    Big Button ring      │ │
  Button GND ───────┤ GP21(LOW) │   │  SK9822 × 11    Big Button accent     │ │
                    │           │   │  RGB LED × 1    Status indicator      │ │
                    └───────────┴───┴──────────────────────────────────────-┘ │
                                                                              │
                    3.3V ── TCA9548A, all OLEDs, HT16K33, SK9822 VCC, RGB LED│
                    5V   ── VSYS (via 1N5817), WS2812 VCC                    │
                    GND  ── star point (LED strip GND direct, not via Pico)  │
                    └────────────────────────────────────────────────────────┘
```

---

## 4. Module Details

### Wires  (Pico 2)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x11)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP10 R    │ RGB status LED                                            │
  │  GP11 G    │                                                           │
  │  GP12 B    │                                                           │
  │  GP26 ADC0 │ Wire sense — resistor ladder (6 wires, unique R each)    │
  │  3V3  out  │ → top of resistor ladder                                  │
  │  GND       │ → bottom of resistor ladder                               │
  └────────────┴──────────────────────────────────────────────────────────┘
```

### Keypad  (Pico 3)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x12)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP4  SDA  │ ─── TCA9548A ─── CH0..CH5 ─── SSD1306 128×64 × 6        │
  │  GP5  SCL  │                  (one symbol per display)                │
  │  GP10 R    │ RGB status LED                                            │
  │  GP11 G    │                                                           │
  │  GP12 B    │                                                           │
  │  GP21      │ Button 1                                                  │
  │  GP22      │ Button 2                                                  │
  │  GP24      │ Button 3                                                  │
  │  GP25      │ Button 4                                                  │
  │  GP26      │ Button 5                                                  │
  │  GP27      │ Button 6                                                  │
  └────────────┴──────────────────────────────────────────────────────────┘
```

### Simon Says  (Pico 4)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x13)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP10 R    │ RGB status LED                                            │
  │  GP11 G    │                                                           │
  │  GP12 B    │                                                           │
  │  GP14      │ Red LED       ┐                                           │
  │  GP24      │ Red button    ┘  paired                                   │
  │  GP15      │ Blue LED      ┐                                           │
  │  GP25      │ Blue button   ┘  paired                                   │
  │  GP16      │ Green LED     ┐                                           │
  │  GP26      │ Green button  ┘  paired                                   │
  │  GP17      │ Yellow LED    ┐                                           │
  │  GP27      │ Yellow button ┘  paired                                   │
  └────────────┴──────────────────────────────────────────────────────────┘
```

### Who's on First  (Pico 5)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x14)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP4  SDA  │ ─── TCA9548A ─── CH0..CH5 ── SSD1306 128×64 × 6 (words)│
  │  GP5  SCL  │                  CH7       ── SSD1306 128×32   (display) │
  │  GP10 R    │ RGB status LED                                            │
  │  GP11 G    │                                                           │
  │  GP12 B    │                                                           │
  │  GP18 SCK  │ SK9822 × 5 (repeat count indicator)                      │
  │  GP19 DATA │                                                           │
  │  GP21      │ Button 1                                                  │
  │  GP22      │ Button 2                                                  │
  │  GP24      │ Button 3                                                  │
  │  GP25      │ Button 4                                                  │
  │  GP26      │ Button 5                                                  │
  │  GP27      │ Button 6                                                  │
  └────────────┴──────────────────────────────────────────────────────────┘
```

### Memory  (Pico 6)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x15)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP10 R    │ RGB status LED                                            │
  │  GP11 G    │                                                           │
  │  GP12 B    │                                                           │
  │  GP18 SCK  │ SK9822 × 5 (repeat count indicator)                      │
  │  GP19 DATA │                                                           │
  │  GP4  SDA  │ ─── TCA9548A ─── CH0..CH4 ── MAX7219 or HT16K33         │
  │  GP5  SCL  │                  (5× 7-seg digits — TBD connection)      │
  │  GP21      │ Button 1                                                  │
  │  GP22      │ Button 2                                                  │
  │  GP24      │ Button 3                                                  │
  │  GP25      │ Button 4                                                  │
  └────────────┴──────────────────────────────────────────────────────────┘
```

### Switches  (Pico 7)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x16)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP10 R    │ RGB status LED                                            │
  │  GP11 G    │                                                           │
  │  GP12 B    │                                                           │
  │  GP14..18  │ 5× SPDT switch inputs (one pin each, pull-up)            │
  │  GP20..27  │ 10× red LEDs (or SK9822 × 10 strip — TBD)               │
  └────────────┴──────────────────────────────────────────────────────────┘
```

### Maze  (Pico 8)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x17)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP10 R    │ RGB status LED                                            │
  │  GP11 G    │                                                           │
  │  GP12 B    │                                                           │
  │  GP18 SCK  │ SK9822 × 36  (6×6 grid)   ← keep GND wire short & heavy │
  │  GP19 DATA │                                                           │
  │  GP24      │ Up button                                                 │
  │  GP25      │ Down button                                               │
  │  GP26      │ Left button                                               │
  │  GP27      │ Right button                                              │
  └────────────┴──────────────────────────────────────────────────────────┘
```

### Password  (Pico 9)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x18)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP4  SDA  │ ─── TCA9548A ─── CH0..CH4 ── SSD1306 128×32 × 5 (chars)│
  │  GP5  SCL  │                                                           │
  │  GP10 R    │ RGB status LED                                            │
  │  GP11 G    │                                                           │
  │  GP12 B    │                                                           │
  │  GP14,GP15 │ Column 1 up/down buttons                                  │
  │  GP16,GP17 │ Column 2 up/down                                          │
  │  GP19,GP20 │ Column 3 up/down                                          │
  │  GP21,GP22 │ Column 4 up/down                                          │
  │  GP24,GP25 │ Column 5 up/down                                          │
  │  GP26      │ Submit button                                             │
  └────────────┴──────────────────────────────────────────────────────────┘
```

### Venting Gas  (Pico 10)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x19)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP4  SDA  │ ─── SSD1306 128×32 (question display — direct I2C)       │
  │  GP5  SCL  │                                                           │
  │  GP14      │ YES button                                                │
  │  GP15      │ NO button                                                 │
  │  GP20..27  │ 2× 7-seg displays (TBD — MAX7219 or TM1637)             │
  └────────────┴──────────────────────────────────────────────────────────┘
```

### Knob  (Pico 11)
```
  ┌────────────┬──────────────────────────────────────────────────────────┐
  │  GP2  SDA  │ ─── inter-Pico bus (i2c1, slave 0x1A)                     │
  │  GP3  SCL  │ ─── inter-Pico bus                                        │
  │  GP6  ATTN │ ─── shared attention line (pull low when reporting)       │
  │  GP10 R    │ RGB status LED  (if fitted)                               │
  │  GP11 G    │                                                           │
  │  GP12 B    │                                                           │
  │  GP18 SCK  │ SK9822 × 12 (red position indicators)                    │
  │  GP19 DATA │                                                           │
  │  GP14,GP15 │ 2× 7-seg displays (TBD — MAX7219 or TM1637)             │
  │  GP20,GP21 │ 4-pos rotary switch (2-bit Gray code or 4 discrete pins) │
  │  GP22,GP23 │                                                           │
  │  GP24..27  │ "UP" indicator LEDs × 4 (one lights per knob position)   │
  └────────────┴──────────────────────────────────────────────────────────┘
```

---

## 5. Physical Panel Layout

```
  ╔══════════════════════════════════════════════════════════════════════╗
  ║                         FRONT PANEL                                  ║
  ║                                                                      ║
  ║   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              ║
  ║   │   BIG BUTTON │  │    WIRES     │  │    KEYPAD    │              ║
  ║   │  (Central)   │  │   (Pico 2)   │  │   (Pico 3)   │              ║
  ║   │              │  │              │  │              │              ║
  ║   │  WS2812 ring │  │ 6 wire slots │  │ 6 symbol     │              ║
  ║   │  SK9822 strip│  │              │  │   OLEDs +    │              ║
  ║   │  OLED label  │  │              │  │   buttons    │              ║
  ║   └──────────────┘  └──────────────┘  └──────────────┘              ║
  ║                                                                      ║
  ║   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              ║
  ║   │  SIMON SAYS  │  │ WHO'S ON 1ST │  │    MEMORY    │              ║
  ║   │   (Pico 4)   │  │   (Pico 5)   │  │   (Pico 6)   │              ║
  ║   │              │  │              │  │              │              ║
  ║   │ 4 color LEDs │  │ 6 word OLEDs │  │ 5× 7-seg     │              ║
  ║   │ 4 buttons    │  │ 6 buttons    │  │ 4 buttons    │              ║
  ║   │              │  │ SK9822 ×5    │  │ SK9822 ×5    │              ║
  ║   └──────────────┘  └──────────────┘  └──────────────┘              ║
  ║                                                                      ║
  ║   ┌────────────────────────────────────────────────────────────────┐ ║
  ║   │     TIMER (HT16K33)   SERIAL (OLED)   STRIKES (OLED)          │ ║
  ║   │     BATTERY × 2       INDICATOR        [Central displays]      │ ║
  ║   └────────────────────────────────────────────────────────────────┘ ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                   [  physical key lock  ]                            ║
  ╠══════════════════════════════════════════════════════════════════════╣
  ║                         BACK PANEL                                   ║
  ║                                                                      ║
  ║   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              ║
  ║   │   SWITCHES   │  │     MAZE     │  │   PASSWORD   │              ║
  ║   │   (Pico 7)   │  │   (Pico 8)   │  │   (Pico 9)   │              ║
  ║   │              │  │              │  │              │              ║
  ║   │ 5 switches   │  │ SK9822 ×36   │  │ 5× char OLED │              ║
  ║   │ 10 red LEDs  │  │ (6×6 grid)   │  │ 10 buttons   │              ║
  ║   │              │  │ 4 dir buttons│  │ + submit     │              ║
  ║   └──────────────┘  └──────────────┘  └──────────────┘              ║
  ║                                                                      ║
  ║   ┌──────────────┐  ┌──────────────┐                                 ║
  ║   │ VENTING GAS  │  │     KNOB     │                                 ║
  ║   │  (Pico 10)   │  │  (Pico 11)   │                                 ║
  ║   │              │  │              │                                 ║
  ║   │ OLED display │  │ 4-pos knob   │                                 ║
  ║   │ YES/NO btns  │  │ SK9822 ×12   │                                 ║
  ║   │ 2× 7-seg     │  │ 2× 7-seg     │                                 ║
  ║   └──────────────┘  └──────────────┘                                 ║
  ╚══════════════════════════════════════════════════════════════════════╝
```

---

## 6. Wire Gauge Guide

| Connection | Gauge | Reason |
|---|---|---|
| Battery → switch → buck converter | 18 AWG | Up to 3 A load |
| 5V bus backbone | 18 AWG | Full system load |
| 3.3V bus backbone | 20 AWG | Logic only, < 500 mA |
| Per-Pico 5V drop from bus | 22 AWG | < 100 mA per Pico |
| Maze SK9822 GND return | 18 AWG | Up to 720 mA peak |
| Other SK9822/WS2812 GND | 20 AWG | < 300 mA |
| Bus signal wires (SDA/SCL/ATTN) | 24–26 AWG | Signal only; keep the run under ~1 m |
| I2C within a module | 24–26 AWG | Signal only |
| GND bus spine | 16–14 AWG | All returns, keep short |
