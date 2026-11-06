# Hardware Build Guide

How the bomb is physically wired: the inter-Pico bus board, the power
distribution board, and how each Pico connects to them.

Pin-level detail lives elsewhere: per-module pinouts in `SYSTEM_DIAGRAM.md` §4,
the central's pinout in `central_controller/WIRING.md`, the standard module pin
table in `SETUP.md` §6. This document is about *construction*: what to buy, how
it goes together, and why.

---

## 1. The approach in one picture

The bomb is a single two-sided assembly: modules mount on the front face and the
back face, and everything shared lives in the middle cavity. There are **two
distribution boards, and no splices anywhere**.

```
 ┌──────────────┐      ┌───────────────────┐
 │ 7.4 V SIDE   │ 5 V  │   POWER BOARD     │  3-wire power drop
 │ pack, fuse,  ├─────►│   (the star)      ├──────────────────────► each Pico
 │ switch, buck │ 3V3  │  5V | 3V3 | GND   │  (5V, 3V3, GND)
 │ LDO          │      └────────┬──────────┘
 └──────────────┘               │ 3V3 + GND anchor
                        ┌───────▼───────────┐
                        │    BUS BOARD      │  4-wire signal cable
                        │    J1 … J12       ├──────────────────────► each Pico
                        └───────────────────┘  (ATTN, GND, SDA, SCL)
```

Every Pico therefore has **two cables**: a 4-wire signal cable to the bus board
and a 3-wire power drop to the power board. Each cable does one job.

Why boards and not cut-and-solder splices: roughly 40 three-wire splices for
the signals alone, each a place for an intermittent fault; one bad joint can
take out every module past it on a shared bus. You also could not pull a module
without desoldering. Solder wicks into stranded wire and leaves a stiff spot
that fatigues when the prop is handled.

---

## 2. Inter-Pico bus board

A perfboard spine in the middle cavity. One header per module; all the pins of
the same function are tied together, and those tied rows *are* the bus.

### 2.1 Header pinout (4 pins)

| Header pin | Signal | Pico pin |
|------------|--------|----------|
| 1          | ATTN   | 2 (GP1)  |
| 2          | GND    | 3        |
| 3          | SDA    | 4 (GP2)  |
| 4          | SCL    | 5 (GP3)  |

The order matches the Pico's own pad order, so a cable solders on in a straight
run with no crossed wires. (Putting GND between SDA and SCL buys nothing at
100 kHz with sub-30 cm stubs, so the pinout follows the pads instead.)

**There is no power on this header.** A miswired plug can only short a 3.3 V
signal to ground, which damages nothing. All headers are identical, so any
module can go in any socket; a module's address is set in its firmware, not by
position.

### 2.2 Header assignment

| Header | Module         | ID   | Address |
|--------|----------------|------|---------|
| J1     | Central        | 0x00 | master  |
| J2     | Wires          | 0x01 | 0x11    |
| J3     | Keypad         | 0x02 | 0x12    |
| J4     | Simon Says     | 0x03 | 0x13    |
| J5     | Who's on First | 0x04 | 0x14    |
| J6     | Memory         | 0x05 | 0x15    |
| J7     | Switches       | 0x06 | 0x16    |
| J8     | Maze           | 0x07 | 0x17    |
| J9     | Password       | 0x08 | 0x18    |
| J10    | Venting Gas    | 0x09 | 0x19    |
| J11    | Knob           | 0x0A | 0x1A    |
| J12    | spare / logic analyzer | — | —   |

Header number is module ID + 1. Put the front-face modules (J1–J6) on one edge
of the board and the back-face modules (J7–J11) on the other, so every stub
stays short. This is a convention for tidiness, not a requirement.

### 2.3 What is on the board

```
 3V3 (from power board) ──┬───────┬───────┐
                        [3.3k]  [3.3k]  [10k]       ← 2 footprints per line,
                          │       │       │            so values can be tuned
 SDA  ────────────────────┴───────┼───────┼────────────────── to every header
 SCL  ────────────────────────────┴───────┼────────────────── to every header
 ATTN ────────────────────────────────────┴────────────────── to every header
 GND  ═══ short heavy wire to the GND block (the star) ══════ to every header
                         J1   J2   J3  …  J11   J12
```

- **Pull-ups live here, once.** 3.3 kΩ on SDA and SCL, 10 kΩ on ATTN (ATTN is
  slow, so a weaker pull-up is fine). Never populate pull-up footprints on a
  module's own breakout: ten in parallel is 470 Ω and the bus stops pulling low.
- **Service feed.** A 2-wire feed to the power board: 3V3 for the pull-ups, and
  GND as a short heavy anchor to the star (see §4.6). This keeps the signal-GND
  network tied to the star instead of floating on the thin stubs.
- **Test pads** for SDA, SCL, ATTN and GND, each with a GND point close by so a
  scope probe has a short ground path.
- **Optional:** a red LED with a 1 kΩ resistor from 3V3 to ATTN lights whenever
  any module is asserting ATTN. A free "someone is shouting" indicator while
  debugging.

### 2.4 Why 3.3 kΩ, and why stubs stay short

I2C is limited by total capacitance, and capacitance sums over **all** the
wire, stubs included, so a star of 11 stubs adds more wire than a linear run.
At 100 kHz the rise time may be at most 1 µs, and `t_rise ≈ 0.8473 · R · C`:

| Pull-up | Max bus capacitance |
|---------|---------------------|
| 4.7 kΩ  | ≈ 250 pF            |
| 3.3 kΩ  | ≈ 360 pF            |

Multi-conductor cable runs roughly 50–100 pF per metre and each Pico adds about
10 pF (both are estimates). With 11 stubs plus the spine you can land near
300 pF, which is why 3.3 kΩ is the starting value. Keep stubs **30 cm or less**
(10–20 cm is realistic in a compact assembly). 3.3 kΩ sinks about 1 mA, well
under the 3 mA I2C limit. Confirm on a scope once built (§8).

### 2.5 Connector: screw terminals

Use a 4-position screw terminal per header position.

- **5.08 mm pitch** if you are on 2.54 mm perfboard: it is exactly two holes. 3.5
  and 3.81 mm pitch do not land on the grid. A 4-position header is about 20 mm
  wide, so 12 of them need about 24 cm of edge (about 12 cm per edge when
  split across two).
- **Pluggable** (PCB header plus a plug that holds the wires) is recommended: a
  module comes out in one move. Fixed terminals are fine if modules rarely come
  out, but each removal means releasing and re-seating four wires.
- Check the part's wire range covers 24 AWG, and that it is rated well above the
  few milliamps this carries.

**Wire prep.** Crimp a ferrule on every wire (0.25 mm² for 24 AWG) and strip to
the length the part specifies. Do **not** solder-tin the ends: solder creeps
under screw pressure and the joint loosens. Tighten, then tug-test each wire.
Zip-tie the cable a few centimetres from the terminal so a pull hits the tie,
not the terminal.

### 2.6 The one real hazard: wire order

A keyed connector cannot be plugged in wrong. Loose wires in terminals can. If
SDA and SCL get swapped on one module, that module drives the wrong line and can
hold the bus low for **everyone**, which is a miserable fault to trace.

- Use the pluggable type so the order is set once.
- Write the colour-to-signal map on the board.
- Do not give a signal the red wire. Red means power, and the power board sits
  next to this one.

---

## 3. Connecting a Pico

Solder the wires **directly** to the Pico. Dupont pins vibrate loose and are the
weakest link in a prop. Slide-on connectors are not used at the Pico end; the
pluggable connection is at the bus board.

| Cable wire | Pico pad |
|------------|----------|
| ATTN       | pin 2 (GP1) |
| GND        | pin 3 |
| SDA        | pin 4 (GP2) |
| SCL        | pin 5 (GP3) |

Pins 2–5 are one unbroken run of pads at the USB end of the board. Route the
cable away from the micro-USB connector. A soldered Pico is reflashed over USB,
so that port and the BOOTSEL button must stay reachable; if a Pico ends up
buried, bring a short USB extension out to somewhere accessible.

**Strain relief matters more than the solder joint.** A soldered wire fails at
the edge of the joint where solder meets flexible wire, and can lift the pad.

- Clamp or zip-tie the cable to a standoff or the chassis a few centimetres from
  the Pico, with a little slack between the clamp and the joint.
- Pass each wire through its hole before soldering, rather than tacking it to the
  pad.
- Hot glue is a reasonable *second* layer: do it last, after everything is
  tested; use **low-temp** glue (a standard-temp gun can soften thin PVC
  insulation); keep it off the micro-USB port and BOOTSEL button.
- Adhesive-lined heat-shrink over the bundle just past the joint is a tidier
  alternative and gives a stiffness gradient.

---

## 4. Power distribution board

### 4.1 Why a board, not a backbone

With everything fanning out from one middle cavity, the **power board is the
star**. Each module gets its own drop straight from it, so there are no taps and
no daisy-chained wire. The "backbone" shrinks to the short thick feed from the
supply to this board.

Voltage drop is not the reason: 1 A through 1 m of 18 AWG (≈ 21 mΩ/m) costs about
21 mV each way, and 24 AWG is ≈ 84 mΩ/m. The reason is mechanical reliability
and a single place to measure and debug.

### 4.2 Parts and port count

Three **dual-row barrier strips**, 14 positions each, one per rail (5V, 3V3,
GND). Dual-row gives two screws per position. Fit a jumper bar to each strip so
it is a single net. Cheap strips vary in how they are connected internally, so
**check each with a multimeter before wiring**.

| Rail | Ports needed | Contents |
|------|--------------|----------|
| GND  | about 19 | battery −, buck, LDO, bus-board anchor, 11 Pico drops, about 4 heavy strip returns |
| 5V   | about 14 | buck output, LDO input, 11 Pico drops, central WS2812 |
| 3V3  | about 12 | LDO output plus the modules and strips that use it |

### 4.3 Column map

Mount the three strips side by side. **The same column is the same module on all
three rails**, so tracing a fault means reading across a column. Top screws take
the module drops; bottom screws take the extras (heavy LED returns).

| Column | Contents |
|--------|----------|
| 1      | Source: battery −, buck, LDO, bulk capacitors |
| 2–3    | Central (it has the most wires) |
| 4–13   | The ten modules: column = module ID + 3 (Wires = 4 … Knob = 13) |
| 14     | Bus-board service feed (3V3 + GND anchor) |

### 4.4 Terminating wires

Barrier strips are designed for ring or fork lugs. Use a lug or ferrule sized to
the wire: 0.25 mm² for 24 AWG, 1.0 mm² for 18 AWG. Bare stranded wire under a
screw squashes out and works loose. Do not solder-tin. Label every wire at both
ends.

### 4.5 Keep 7.4 V off this board

The battery, fuse, switch and buck input are a **separate small junction**
(Wago levers are ideal), well away from these strips. If a slip ever puts 7.4 V
on a 5 V terminal, every Pico dies at once.

### 4.6 Star ground rules

1. Battery −, buck GND, LDO GND and every module GND land on the GND strip. The
   bus board's GND anchor lands there too, on a short heavy wire. That is what
   keeps the small DC loop through the signal cables harmless.
2. Heavy returns (Maze, Knob strip, the central's WS2812 and SK9822) get their
   own wire straight to the GND strip. Never route them through a Pico GND pin or
   through the thin signal cable.
3. Put the heaviest loads nearest the source column.

### 4.7 The per-module drop

A 3-wire power drop (5V, 3V3, GND) in 24 AWG. The fourth conductor of a
4-conductor cable can be a second GND to halve the return resistance. Wire only
the rails a module uses: Wires and Simon Says drive their LEDs from GPIOs and
need just 5V and GND.

Heavy loads get their own 20–22 AWG pair straight from the strips (Maze, Knob,
and the central's WS2812/SK9822), not the shared drop.

**At the Pico end:**

- 5V goes through a **1N5817** (anode to the rail, band toward the Pico) to VSYS.
- The 3V3 rail goes only to the peripherals' VCC. **Never connect it to the
  Pico's own 3V3(OUT) pin**: two regulators fighting on one node.
- GND goes to a Pico GND pin.

### 4.8 Capacitors

About 470 µF across each rail on the source column, plus about 100 µF near any
LED strip.

---

## 5. Supply checks

- **Fuse.** Put a 3–5 A fuse in the pack positive, before the switch. Two 18650s
  can push far more current than any of this wire survives; the fuse is the only
  thing between a wiring mistake and a fire.
- **Protection board.** A TP5100 module is, as far as I know, only a charger: no
  cell balancing and no over-discharge or short-circuit cutoff. Confirm against
  your module's datasheet, and make sure the pack has a separate 2S protection
  board with short-circuit cutoff.
- **Buck current limit.** If your XL4016 module has a current-limit pot, set it
  to about 3 A so a shorted 5 V rail cannot draw the full 8 A.
- **3.3 V rail.** The AMS1117 is rated 1 A and carries all the OLEDs and LED
  strips. Typical load is well under that, but the Maze strip alone can peak near
  735 mA, and the LDO turns 1.7 V × current into heat. Feel its temperature
  during testing; a 5→3.3 V buck module is a drop-in swap if it runs hot.

---

## 6. Power on/off and the back-panel key

- **Power every Pico together.** A Pico with no power can clamp SDA and SCL
  through its protection diodes and drag the whole bus down. Do not plug or
  unplug bus connectors while the bomb is powered.
- **The key is purely mechanical.** It opens the back panel and is not wired to
  anything. The back-panel Picos stay powered all the time and are disabled in
  software by the level's `active_modules` mask.
- **Known gap:** a module that is not active in the current level receives no
  messages at all, so it sits in its power-on idle state (white status LED)
  instead of looking dead. To switch the LED off, the central would send those
  modules `MSG_LOCK`; the module side already handles it, but the central does not
  send it yet.

---

## 7. Bring-up checklist

Before the pack touches anything:

1. Probe each barrier strip with a multimeter to confirm how its positions are
   connected, then fit the jumper bars.
2. With every drop wired, check 5V–GND, 3V3–GND and 5V–3V3. The capacitors make
   the meter drift, but nothing should read near zero.
3. Power the buck alone with no drops connected and trim it to **5.00 V**. Check
   the LDO reads 3.3 V.
4. Add loads gradually, not all 11 modules at once. If you have a bench supply,
   use it with a current limit around 500 mA for the first power-up.
5. Flash and power the central alone and run the Big Button test level before
   adding a second Pico to the bus.
6. Add modules one at a time to the bus board.

---

## 8. Still to verify on hardware

- Scope SDA and SCL with all 11 connected: confirm rise times and tune the
  pull-ups (3.3 kΩ is the starting point, not a measured value).
- AMS1117 temperature under load.
- That the chosen screw terminals accept a 24 AWG ferrule and hold a tug test.
- The I2C slave handler in `shared/bus_protocol.c` on real silicon. It compiles,
  but the ISR and ring-buffer handoff have never run on hardware.
