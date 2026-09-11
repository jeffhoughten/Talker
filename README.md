# Talker

A physical, hardware implementation of the bomb-defusal puzzle game
*Keep Talking and Nobody Explodes*, built from **11 Raspberry Pi Picos**.

One Pico runs the game — timer, strikes, level progression, and the Big Button
module. Ten more each run a single puzzle module and report back over a shared
bus. Firmware is C against the **Raspberry Pi Pico SDK**.

---

## Status

Working and on hardware:

- Central controller: game state machine, countdown, strike tracking, level progression
- Seven displays driven through a TCA9548A mux (HT16K33 7-segment + six SSD1306 OLEDs)
- Big Button module with the full KTANE rule engine
- Per-game randomisation of batteries, indicators, serial number, and which
  physical screen shows what
- Level progress saved to flash
- Inter-Pico I2C bus protocol, both master and slave sides

Not done yet:

- Nine of the ten module firmwares are scaffolds — only Wires is written
- Win/lose LED animations
- Level-select UI
- The HOLD branch of the Big Button scenario randomiser

---

## How the Picos talk

A single **I2C multi-drop bus**. The central controller is the master on `i2c1`
(GP2/GP3); every module is a slave at `0x10 + module_id`, so Wires is `0x11`
through Knob at `0x1A`. All eleven boards tap the same four wires in parallel —
there is no chain, and pulling one module does not isolate the others.

An I2C slave cannot start a transfer, so a module with something to report
(a strike, a solve) queues the message and pulls a shared open-drain **ATTN**
line on GP6 low. The central sees it and sweeps the bus to collect. A slow
round-robin poll runs underneath as a backstop, so an unwired ATTN costs
latency rather than silence.

> Wiring gotcha: the bus pull-ups — 4.7 kΩ on SDA, SCL and ATTN — go on **once**,
> at the central. Populating them per module puts ten in parallel and the bus
> stops working.

The protocol lives in [`shared/bus_protocol.h`](shared/bus_protocol.h).

---

## Layout

```
CMakeLists.txt            top-level build; one target per Pico
shared/                   code every Pico links
  bus_protocol.{h,c}        I2C bus: framing, CRC, master + slave
  module_base.{h,c}         module lifecycle, status LED, RNG
central_controller/       the game Pico
  main.c                    state machine, displays, Big Button
  game_state.{h,c}          game rules and module tracking
  level_config.h            level times, strike limits, active modules
  ht16k33.{h,c}             7-segment timer driver
  ssd1306.{h,c}             OLED driver + font + battery graphics
  save.{h,c}                level progress in flash
modules/<name>/           one folder per puzzle module
```

---

## Documentation

| Document | Covers |
|----------|--------|
| [SETUP.md](SETUP.md) | Toolchain install, building, flashing, bus wiring |
| [SYSTEM_DIAGRAM.md](SYSTEM_DIAGRAM.md) | Full system diagrams, per-module pinouts, panel layout, wire gauges |
| [central_controller/WIRING.md](central_controller/WIRING.md) | Central board pinout, displays, Li-ion power system and grounding |
| [KTANE_COMPLETE_BRIEF.md](KTANE_COMPLETE_BRIEF.md) | Original requirements. Predates the I2C switch — treat its protocol section as historical |

---

## Building

Needs the Pico SDK **1.3.0 or newer** — the modules run as I2C slaves, which
requires `pico_i2c_slave`. Built and verified against **SDK 2.3.1** with
**Arm GNU Toolchain 14.2.Rel1**.

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Produces 11 `.uf2` images, one per Pico. Hold BOOTSEL while plugging in a Pico
and drag the matching file onto the `RPI-RP2` drive.

Toolchain setup — including the two things that reliably trip people up on
Windows (no native ARM64 toolchain, and picotool wanting a host compiler) — is
in [SETUP.md](SETUP.md).
