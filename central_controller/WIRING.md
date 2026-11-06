# Central Controller — Wiring Diagram

## Raspberry Pi Pico

```
                        ┌─────────────────────────────────┐
                        │       RASPBERRY PI PICO         │
                        │           (TOP VIEW)            │
                        │                           [USB] │
                        ├─────────────────────────────────┤
                        │                                 │
                        ┤ GP0   1 ●             ● 40  VBUS ──── USB 5V in
  BUS ATTN     ─────────┤ GP1   2 ●             ● 39  VSYS ──── Battery / diode-OR
                    ────┤        3  GND          ● 38  GND  ─┐
  BUS SDA (i2c1) ───────┤ GP2   4 ●             ● 37  3V3_EN │
  BUS SCL (i2c1) ───────┤ GP3   5 ●             ● 36  3V3  ──┤── 3.3 V rail (to TCA, OLEDs)
  I2C0 SDA     ─────────┤ GP4   6 ●             ● 35  AREF   │
  I2C0 SCL     ─────────┤ GP5   7 ●             ● 34  GP28   │
                    ────┤        8  GND          ● 33  GND  ──┘
                        ┤ GP6   9 ●             ● 32  GP27
                        ┤ GP7  10 ●             ● 31  GP26
                        ┤ GP8  11 ●             ● 30  RUN
                        ┤ GP9  12 ●             ● 29  GP22
                    ────┤       13  GND          ● 28  GND
  RGB Status R  ────────┤ GP10 14 ●             ● 27  GP21 ──── Big Button GND (driven LOW)
  RGB Status G  ────────┤ GP11 15 ●             ● 26  GP20 ──── Big Button (momentary)
  RGB Status B  ────────┤ GP12 16 ●             ● 25  GP19 ──── SK9822 DATA
                        ┤ GP13 17 ●             ● 24  GP18 ──── SK9822 CLK
                    ────┤       18  GND          ● 23  GND
                        ┤ GP14 19 ●             ● 22  GP17
  WS2812 DATA   ────────┤ GP15 20 ●             ● 21  GP16
                        │                                 │
                        └─────────────────────────────────┘
```

**GP0, GP6, GP8 and GP9 are free.** GP0, GP8 and GP9 were daisy-chain UART pins
before the switch to I2C. GP1, the other UART0 pin, is now the ATTN line.

**The four bus signals are on pins 2–5** — ATTN (GP1), GND (pin 3), SDA (GP2),
SCL (GP3) — one unbroken run of pads, so a 4-conductor cable solders on without
crossing wires. They are at the USB end of the board; route the cable away from
the connector so USB stays reachable for reflashing.

**Two I2C buses, and they must not be confused.**

| Bus  | Pins      | Role                                           | Pull-ups |
|------|-----------|------------------------------------------------|----------|
| i2c1 | GP2 / GP3 | Inter-Pico game bus — master to 10 modules     | 3.3k on the bus board |
| i2c0 | GP4 / GP5 | This board's own displays via TCA9548A         | on the breakouts |

Never hang a display off GP2/GP3. Display addresses (0x3C, 0x70, 0x71) do not
currently collide with module addresses (0x11–0x1A), but mixing the two buses
means a display glitch can stall the game bus and vice versa.

---

## I2C Bus Expansion — TCA9548A Multiplexer

The TCA9548A sits on I2C0 (GP4/GP5) at address **0x70**.
It exposes 8 independent I2C channels, each routed to one display.
Channels 1/2 and 3/7 are **randomly swapped** each game start by `scenario_randomize()`.

```
                    3.3V ──┬── VIN
                           │
                    GND ───┤── GND
                           │
  GP4 (SDA) ───────────────┤── SDA    ┌──────────────┐
  GP5 (SCL) ───────────────┤── SCL    │  TCA9548A    │
                           │          │  addr 0x70   │
                    3.3V ──┤── A0     │              │   ← A0 bridged → addr 0x71
                    GND ───┤── A1     │  CH0 ────────┼──── HT16K33  7-seg    (0x71)  TIMER
                    GND ───┤── A2     │  CH1 ────────┼──── SSD1306  128×64  (0x3C)  BATTERY  ← randomised
                    3.3V ──┤── RESET  │  CH2 ────────┼──── SSD1306  128×64  (0x3C)  BATTERY  ← randomised
                           │          │  CH3 ────────┼──── SSD1306  128×32  (0x3C)  SERIAL   ← randomised
                           └──────────┤  CH4 ────────┼──── SSD1306  128×64  (0x3C)  STRIKES
                                      │  CH5 ────────┼──── (unused)
                                      │  CH6 ────────┼──── SSD1306  128×64  (0x3C)  BIG BUTTON LABEL
                                      │  CH7 ────────┼──── SSD1306  128×32  (0x3C)  INDICATOR ← randomised
                                      └──────────────┘

  Randomised pairs (swapped by ROSC each game):
    CH1 ↔ CH2  :  the two battery displays
    CH3 ↔ CH7  :  serial number and indicator
```

> **Address conflict resolved:** A0 on the HT16K33 backpack is bridged to 3.3V,
> setting its address to **0x71** — no longer conflicts with the TCA9548A (0x70).

---

## RGB Status LED

Common-cathode RGB LED (one resistor per leg, ~220 Ω for 3.3 V logic).

```
  GP10 ──── 220Ω ──── R ──┐
  GP11 ──── 220Ω ──── G ──┤── (common cathode) ──── GND
  GP12 ──── 220Ω ──── B ──┘
```

| State      | Color  |
|------------|--------|
| Idle       | White  |
| Active     | Blue   |
| Solved     | Green  |
| Strike     | Red    |
| Locked     | Off    |

---

## Big Button

Momentary push button.  GP21 is driven permanently LOW by firmware
so it acts as the ground pin, avoiding a dedicated GND wire to the
button if routing is tight.

```
  GP20 ──── [BUTTON] ──── GP21 (LOW)
```

> Moved from GP18, which is now SPI0 SCK for the SK9822 strip.
> `BUTTON_GND_PIN` in `main.c` if you wire it elsewhere.

The button is read as active-low with the Pico's internal pull-up enabled on GP20.

---

## WS2812 LED Strip — Big Button Color Ring (12 LEDs)

```
  GP15 ──── DIN  [WS2812 × 12]  DOUT ──── (not connected / next strip if chained)
  3.3V ──── VCC
  GND  ──── GND
```

> WS2812 is nominally 5 V but almost always works at 3.3 V for short strips.
> If colors are wrong or LEDs flicker, add a 74AHCT125 level-shifter on the
> data line (3.3 V → 5 V) and power the strip from VBUS.

---

## SK9822 LED Strip — Big Button Accent Strip (11 LEDs)

SK9822 is SPI-based (clock + data).

```
  GP18 ──── CLK  [SK9822 × 11]
  GP19 ──── DAT
  3.3V ──── VCC
  GND  ──── GND
```

> Moved off GP2/GP3 — those are the inter-Pico I2C bus now.

---

## Inter-Pico Bus Connection (to the bus board)

The central controller is the I2C **master**, and it connects to the bus board
exactly like every other Pico: one 4-wire cable of 30 cm or less to **header J1**.
Every module is a slave at `0x10 + module_id`. The bus board is a perfboard spine
in the middle cavity with one 4-pin header per module, so there are no splices
and no ordering; any module can be unplugged without affecting the rest.
Construction details are in [`../HARDWARE_BUILD.md`](../HARDWARE_BUILD.md).

| Central pin | Signal | Bus-board header pin |
|-------------|--------|----------------------|
| 2 (GP1)     | ATTN   | 1                    |
| 3 (GND)     | GND    | 2                    |
| 4 (GP2)     | SDA    | 3                    |
| 5 (GP3)     | SCL    | 4                    |

```
 3V3 (power board) ──┬───────┬───────┐
                   [3.3k]  [3.3k]  [10k]      ← ONLY here, on the bus board
                     │       │       │
 J1 Central ── SDA ──┴───────┼───────┼───── … to J2–J11 (the ten modules)
               SCL ──────────┴───────┼───── …
               ATTN ─────────────────┴───── …
               GND ═══ short heavy wire to the GND strip (the star) ═══ …
```

| Module          | ID   | Address | Header |
|-----------------|------|---------|--------|
| Central         | 0x00 | master  | J1     |
| Wires           | 0x01 | 0x11    | J2     |
| Keypad          | 0x02 | 0x12    | J3     |
| Simon Says      | 0x03 | 0x13    | J4     |
| Who's on First  | 0x04 | 0x14    | J5     |
| Memory          | 0x05 | 0x15    | J6     |
| Switches        | 0x06 | 0x16    | J7     |
| Maze            | 0x07 | 0x17    | J8     |
| Password        | 0x08 | 0x18    | J9     |
| Venting Gas     | 0x09 | 0x19    | J10    |
| Knob            | 0x0A | 0x1A    | J11    |

**Exactly one set of pull-ups, on the bus board:** 3.3 kΩ on SDA and SCL, 10 kΩ
on ATTN. Do not populate the pull-up footprints on module breakouts. Ten in
parallel drops the effective pull-up to a fraction of one and the bus stops
pulling low cleanly. The values are a starting point to verify on a scope
(HARDWARE_BUILD.md §2.4).

**ATTN** is how a module gets the central's attention, since an I2C slave
cannot start a transfer. It is open-drain: a module with a queued message
switches GP1 to an output driving low, and back to a high-Z input when its
outbox empties. No module ever drives it high, so any number can assert at
once. See `SETUP.md` for the full explanation.

**Common ground is mandatory.** The bus is single-ended and referenced to it.
The bus board's GND ties to the power board's GND strip with a short heavy wire.

---

## Power System — Full Bomb (11 Picos)

---

### Whole-System Current Budget

Per-component assumptions:
- Pico (active): **25 mA**
- SSD1306 128×64: **5 mA** · SSD1306 128×32: **4 mA**
- HT16K33 7-seg digit: **15 mA** · TCA9548A: **1 mA**
- Discrete LED (Simon, status RGB, Switches): **10 mA**
- SK9822 / WS2812: **20 mA per LED** worst-case; gameplay typical ≈ 15% of LEDs lit at moderate brightness
- 7-seg display module (Venting Gas, Memory, Knob): **20 mA** each

```
┌──────────────────┬──────────────────────────────────────────────────┬──────────┬──────────┐
│ Module           │ Hardware                                         │ Typical  │  Peak    │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Central          │ Pico + TCA + HT16K33 + 6× OLED + RGB LED        │  105 mA  │  115 mA  │
│ (Big Button)     │ WS2812 × 12 (button ring)                        │   40 mA  │  240 mA  │
│                  │ SK9822 × 11 (accent strip)                       │   30 mA  │  220 mA  │
│                  │                                           SUBTOTAL│  175 mA  │  575 mA  │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Wires            │ Pico + RGB LED                                   │   35 mA  │   40 mA  │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Keypad           │ Pico + TCA + 6× OLED 128×64 + RGB LED           │   96 mA  │  105 mA  │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Simon Says       │ Pico + RGB LED + 4× discrete LED                 │   75 mA  │   85 mA  │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Who's on First   │ Pico + TCA + 7× OLED + RGB LED                   │   97 mA  │  106 mA  │
│                  │ SK9822 × 5 (repeat indicator)                    │    8 mA  │  100 mA  │
│                  │                                           SUBTOTAL│  105 mA  │  206 mA  │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Memory           │ Pico + RGB LED + 5× 7-seg                        │   80 mA  │   90 mA  │
│                  │ SK9822 × 5 (repeat indicator)                    │    8 mA  │  100 mA  │
│                  │                                           SUBTOTAL│   88 mA  │  190 mA  │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Switches         │ Pico + RGB LED + 10× discrete red LED            │  135 mA  │  135 mA  │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Maze             │ Pico + RGB LED + SK9822 × 36                     │   75 mA  │  735 mA  │
│                  │ (typical: ~8 LEDs lit for walls/player/exit)     │          │  (all on)│
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Password         │ Pico + TCA + 5× OLED 128×32 + RGB LED           │   76 mA  │   85 mA  │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Venting Gas      │ Pico + 2× OLED 128×32 + 2× 7-seg                │   73 mA  │   80 mA  │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ Knob             │ Pico + SK9822 × 12 (red indicators) + 2× 7-seg  │   85 mA  │  280 mA  │
│                  │ + 4× "UP" indicator LEDs                         │          │          │
├──────────────────┼──────────────────────────────────────────────────┼──────────┼──────────┤
│ TOTAL            │                                                  │ ~1 020 mA│~2 375 mA │
└──────────────────┴──────────────────────────────────────────────────┴──────────┴──────────┘
```

**Working figures:**
- **Typical draw: ~1 A** (game in progress, a few LEDs lit per module)
- **Realistic peak: ~1.5 A** (several modules flashing simultaneously)
- **Absolute peak: ~2.4 A** (every SK9822/WS2812 at full white — never happens in gameplay)

> The Maze's 735 mA absolute peak is the outlier. In practice the maze grid shows
> only player position (1 LED), exit (1 LED), and navigated path (~4–6 LEDs) — real
> draw is under 150 mA. Limit SK9822 global brightness in firmware to cap the ceiling.

---

### Should You Use a Separate Battery for the Picos?

**Short answer: No — one battery, but two regulated rails.**

A second battery adds complexity, weight, two charging circuits, and the risk of
ground loops between panels. The right solution is a single battery with:

1. A **5V main rail** from the battery → LED strips + Pico VSYS pins
2. A **single external 3.3V LDO** fed from the 5V rail → all logic (OLEDs, TCA, HT16K33)

This gives the same noise isolation as two batteries — the LDO's output stage
rejects LED switching transients before they reach the displays and Pico I/O — without
any of the complications.

---

### Recommended Power Architecture

A single 18650 cell cannot drive this system — 1 A through a boost converter from
3.7 V is only ~3.7 W delivered, and the boost converter itself runs hot near its
current limit. **Use a 2S Li-ion pack (7.4 V nominal) with a buck converter.**

A buck (step-down) converter is ~93% efficient vs ~82% for a boost, and 7.4 V → 5 V
is a very gentle ratio that keeps the converter cool.

#### Recommended components

| Part | Spec | Example / Notes |
|------|------|-----------------|
| 2S Li-ion pack | 7.4 V, 6 000 mAh | Two 18650 cells in series in a holder, or a purpose-built 2S pack |
| Charger | 2S, ≥ 2 A charge | TP5100 module: a charger only, as far as I know (verify its datasheet) |
| BMS / protection board | 2S, ≥ 5 A, short-circuit cutoff | Separate board; provides balancing and over-discharge / short-circuit protection |
| Fuse | 3–5 A, inline | In the pack positive, before the master switch |
| Buck converter | 7–12 V in → 5 V out, ≥ 3 A | LM2596 (3 A) or XL4016 (8 A) module; set output to 5.00 V before connecting |
| AMS1117-3.3 LDO | 5 V in → 3.3 V out, 1 A | One module shared across all logic; TO-220 with small heatsink |
| Master switch | DPDT slide, ≥ 3 A | Breaks both B+ and 5V rail simultaneously |
| 1N5817 Schottky | 40 V / 1 A | Back-feed protection on each Pico's VSYS pin |

---

### Power Chain Diagram

```
  USB-C ──────────────────────────────────────────────────┐
                                                          ▼
                                                  ┌──────────────┐
                                                  │  TP5100      │  2S charger only
                                                  │  + 2S BMS    │  (BMS is a separate board)
                                                  └──────┬───────┘
                                                    B+   │   B−
                                              ┌──────────┴────────┐
                                              │  2S 18650 PACK    │  7.4V nominal
                                              │  6 000 mAh        │  (2× 3 000 mAh cells)
                                              └──────────┬────────┘
                                                         │ 7.4V
                                                  [ 3–5 A FUSE ]  in the pack positive
                                                         │
                                                  ┌──────┴──────┐
                                                  │   MASTER    │  DPDT slide switch
                                                  │   SWITCH    │  rated ≥ 3 A
                                                  └──────┬──────┘
                                                         │ 7.4V
                              ┌──────────────────────────┤
                              │                          │
                              ▼                          ▼
                    ┌──────────────────┐      ┌──────────────────┐
                    │   Buck converter │      │  AMS1117-3.3 LDO │  (fed from 5V rail)
                    │  7.4V → 5.00V    │      │   5V → 3.3V      │
                    │  XL4016 / LM2596 │      │   max 1A         │
                    │  set w/ trim pot │      │   add heatsink   │
                    └────────┬─────────┘      └────────┬─────────┘
                             │                         │
                         5V RAIL                   3.3V RAIL
                             │                         │
       ┌─────────────────────┤              ┌──────────┤
       │          │          │              │          │
  1N5817 ▶   1N5817 ▶   WS2812            TCA9548A  SSD1306
  Pico 1     Pico 2      VCC          (all modules) (all modules)
  VSYS       VSYS    (& other Picos)  HT16K33      SK9822 VCC
  (×11 total,                         RGB LEDs
   one diode each)                    7-seg modules
```

> **One Schottky diode per Pico VSYS pin.** The diodes prevent any Pico's onboard
> regulator from back-feeding into the 5V rail when that Pico is powered via its
> own USB port during development.

---

### Power Distribution Board

The power board is the **star**. It is three jumpered, dual-row barrier strips
(5V, 3V3, GND) in the middle cavity, and every Pico gets its own 3-wire drop
straight from it. This replaces the earlier idea of a long backbone with a tap
per module: there are no taps and no daisy-chained wire. The column map, parts
list and wiring practice are in [`../HARDWARE_BUILD.md`](../HARDWARE_BUILD.md) §4.

```
  7.4 V junction ─► buck ─► 5V ───┐        column:  1      2-3      4 … 13     14
  (pack, fuse, switch)  LDO ─► 3V3 ┤                source  Central  10 modules  bus-board
                                   ▼                                            feed
          ┌──────────────────────────────────────────────┐
          │  5V   strip  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○ │
          │  3V3  strip  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○ │
          │  GND  strip  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○ │  ◄ the star point
          └───────────────────────────┬──────────────────┘
                 one 3-wire drop (5V, 3V3, GND) per Pico, 24 AWG
```

The same column is the same module on all three rails. Keep the 7.4 V side
(pack, fuse, switch, buck input) on a separate small junction well away from
these strips, so a slip cannot put 7.4 V on the 5 V rail.

At each Pico:

```
  5V rail ──▶|── VSYS (pin 39)        ▶| = 1N5817, anode to the rail, band to the Pico
  3V3 rail ───── peripherals' VCC      never to the Pico's own 3V3(OUT) pin
  GND strip ──── a Pico GND pin
```

Fit about 470 µF across each rail on the source column, plus about 100 µF near
any LED strip.

---

### Ground Plan

A single star ground: **the GND strip on the power board is the one point** where
everything returns. These land on it directly:

- battery −, buck GND (in and out), AMS1117 GND
- each Pico's GND drop (11 drops)
- the bus-board GND anchor, on a short heavy wire (18–20 AWG), which keeps the
  signal-ground network tied to the star
- the high-current LED returns: WS2812 and SK9822 (central), SK9822 (Who's on
  First, Memory, Maze, Knob)

Rules:

1. High-current LED strip returns go **straight to the GND strip on their own
   wire**, never through a Pico GND pin and never through the thin signal cable.
2. Put the heaviest loads nearest the source column.
3. Display and TCA GND pins bond to the nearest Pico GND.

> Use **24 AWG** for the per-Pico power drops (4th conductor as a second GND).
> Use **20–22 AWG** for each LED strip's VCC/GND pair.
> Use **18 AWG** for the Maze SK9822 GND return, the feeds into the power board,
> and the bus-board GND anchor.

---

### Supply Safety Checks

- **Fuse:** 3–5 A in the pack positive, before the master switch.
- **Protection board:** a TP5100 module is, as far as I know, only a charger (no
  cell balancing, no over-discharge or short-circuit cutoff); confirm against its
  datasheet. The pack needs a separate 2S protection board with short-circuit
  cutoff.
- **Buck current limit:** if the XL4016 module has a current-limit pot, set it to
  about 3 A.
- **AMS1117:** rated 1 A and carries the OLEDs and LED strips. Check its
  temperature under load; a 5→3.3 V buck is a drop-in swap if it runs hot.

---

### Runtime Estimate

| Battery | Capacity | Usable at 5V (93% buck) | @ 1A typical | @ 1.5A realistic peak avg |
|---------|----------|------------------------|--------------|--------------------------|
| 2S 4 000 mAh | 29.6 Wh | 27.5 Wh | **5.5 h** | 3.7 h |
| 2S 6 000 mAh | 44.4 Wh | 41.3 Wh | **8.2 h** | 5.5 h |
| 2S 10 000 mAh | 74 Wh | 68.8 Wh | **13.7 h** | 9.2 h |

**Recommendation: 2S 6 000 mAh** — fits two 3 000 mAh 18650 cells in a standard 2S
holder (~$5), gives a full day of casual play without recharging, and keeps the
pack light enough to mount inside the prop enclosure.

> A 2S 6 000 mAh pack at the C/5 discharge rate (1.2 A) is well within the safe
> operating range for any quality 18650 cell. No thermal concerns.
