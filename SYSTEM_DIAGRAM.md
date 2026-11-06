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
                                    │  charger + 2S   │◄──────────────────
                                    │  BMS (separate) │
                                    └────────┬────────┘
                                             │ 7.4V  BAT−
                                    ┌────────┴────────┐   ┌──────────────────┐
                                    │  MASTER SWITCH  │   │  GND STRIP (star)│
                                    │  DPDT  ≥ 3A     │   │  (power board)   │
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
          │    │  (barrier    │   │  All OLEDs, TCA9548A chips, HT16K33,        │   │
          │    │   strips)    │   │  SK9822 VCC, RGB LEDs, 7-seg modules        │   │
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

All eleven Picos share one 4-wire bus: ATTN, GND, SDA, SCL. Physically it is a
**bus board**, a perfboard spine in the middle cavity of the assembly with one
4-pin header per module. Every module connects to its header with a 4-wire cable
of 30 cm or less. Nothing is spliced, there is no ordering, and pulling one
module out does not affect any other. Construction details are in
[HARDWARE_BUILD.md](HARDWARE_BUILD.md).

```
           J1       J2       J3       J4       J5       J6       ← front-face edge
         Central   Wires   Keypad    Simon    Who's   Memory  
            │        │        │        │        │        │    
ATTN    ════╪════════╪════════╪════════╪════════╪════════╪══════ ATTN
GND     ════╪════════╪════════╪════════╪════════╪════════╪══════ GND
SDA     ════╪════════╪════════╪════════╪════════╪════════╪══════ SDA
SCL     ════╪════════╪════════╪════════╪════════╪════════╪══════ SCL
            │        │        │        │        │        │    
        Switches   Maze   Password  Venting   Knob     spare  
           J7       J8       J9       J10      J11      J12      ← back-face edge
```

```
                       3V3 (from the power board, column 14)
                        │
               ┌────────┼────────┬────────┐
             [3.3k]   [3.3k]   [10k]       ← fitted ONCE, on the bus board
               │        │        │
              SDA      SCL      ATTN       GND ═ short heavy wire to the GND strip
```

Slave address is `0x10 + module_id`, set in firmware at compile time. Any module
can go in any header.

### Header pinout (4 pins, no power)

| Header pin | Signal | Pico pin |
|------------|--------|----------|
| 1          | ATTN   | 2 (GP1)  |
| 2          | GND    | 3        |
| 3          | SDA    | 4 (GP2)  |
| 4          | SCL    | 5 (GP3)  |

### The three signals

| Wire | GPIO | Who drives it                     | Idle state | Pull-up |
|------|------|-----------------------------------|------------|---------|
| SDA  | GP2  | master and slaves (open-drain)    | high       | 3.3 kΩ  |
| SCL  | GP3  | master only                       | high       | 3.3 kΩ  |
| ATTN | GP1  | slaves pull low only, never high  | high       | 10 kΩ   |

**Pull-ups go on once, on the bus board.** The temptation is to fit a pair per
module because each breakout has footprints for them. Don't: ten pull-ups in
parallel is a fraction of the resistance of one, which the RP2040's open-drain
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
fallback, not the plan: wire ATTN.

### Bus speed and length

100 kHz, set by `BUS_FREQ_HZ` in `shared/bus_protocol.h`. I2C is limited by total
bus capacitance, which sums over all the wire including every stub, and 400 kHz
gets marginal at this wiring length. The standard-mode rise-time limit of 1 µs
allows roughly 250 pF with 4.7 kΩ pull-ups or 360 pF with 3.3 kΩ, which is why
3.3 kΩ is the starting value. Keep each stub to 30 cm or less. A 20-byte frame at
100 kHz takes about 2 ms, far faster than any human input the game has to catch.
Raise the speed, or change the pull-ups, only after putting a scope on SCL with
all 11 modules connected. The capacitance figures here are estimates, not
measurements.

---

## 3. Central Controller Detail

```
                    ┌────────────────────────────────────────────────────────┐
                    │                  CENTRAL CONTROLLER                    │
                    │                   Raspberry Pi Pico                    │
                    ├───────────┬────────────────────────────────────────────┤
  BUS ATTN ─────────┤ GP1  ATTN │   ← to bus board J1 (cable ≤ 30 cm)        │
  BUS SDA ──────────┤ GP2  SDA  │   ← i2c1 master, to bus board J1           │
  BUS SCL ──────────┤ GP3  SCL  │   ← i2c1 master, to bus board J1           │
                    │           │                                            │
  I2C0 SDA   ───────┤ GP4  SDA ─┼───┤SDA  ┌────────────────────────────────┐ │
  I2C0 SCL   ───────┤ GP5  SCL ─┼───┤SCL  │      TCA9548A  (0x70)          │ │
                    │           │   │     │  CH0 ──── HT16K33 7-seg (0x71) │ │
                    │           │   │     │  CH1 ──── SSD1306 128×64(0x3C) │ │  battery *
                    │           │   │     │  CH2 ──── SSD1306 128×64(0x3C) │ │  battery *
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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
  │  GP1  ATTN │ ─── shared attention line (pull low when reporting)       │
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

The front and back panels are one two-sided assembly; the power board and bus
board sit in the shared middle cavity. The key lock is **purely mechanical**: it
opens the back panel and is not wired to any switch. The back-panel Picos stay
powered at all times and are disabled in software by the level's active-module
mask. Never switch power to a Pico that is on the bus (see HARDWARE_BUILD.md §6).

---

## 6. Wire Gauge Guide

| Connection | Gauge | Reason |
|---|---|---|
| Battery → fuse → switch → buck converter | 18 AWG | Up to 3 A load |
| Buck / LDO → power board (5V, 3V3, GND feeds) | 18 AWG | Full system load, short run |
| Per-Pico power drop (5V, 3V3, GND) | 24 AWG | Typically under 150 mA per Pico; use the 4th conductor as a second GND |
| Maze SK9822 GND return | 18 AWG | Up to 720 mA peak, straight to the GND strip |
| Other SK9822/WS2812 feeds (VCC and GND) | 20–22 AWG | Under 300 mA, own wire pair, not the shared drop |
| Bus-board GND anchor to the star | 18–20 AWG | Short and heavy |
| Bus signal cable (ATTN/GND/SDA/SCL), per module | 24 AWG | Signal only; 30 cm or less |
| I2C within a module | 24–26 AWG | Signal only |

Every Pico has two cables: a 4-wire signal cable to the bus board and a 3-wire
power drop to the power board. See [HARDWARE_BUILD.md](HARDWARE_BUILD.md).
