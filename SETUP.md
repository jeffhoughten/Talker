# Pico C SDK Setup & Build Guide

## 1. Install Prerequisites (Windows)

### ARM toolchain
Download and install the **GNU Arm Embedded Toolchain** from:
https://developer.arm.com/downloads/-/gnu-rm
(Choose the Windows installer, e.g. `gcc-arm-none-eabi-10.3-...-win32.exe`)

Add to PATH: `C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin`

### CMake
Download from https://cmake.org/download/ — choose the Windows installer.
Make sure "Add CMake to PATH" is checked during install.

### Make / Ninja
Install either:
- **Ninja** (recommended): `winget install Ninja-build.Ninja`
- **make** via Git for Windows or MSYS2

### Git
`winget install Git.Git` if not already installed.

---

## 2. Clone the Pico SDK

```
git clone https://github.com/raspberrypi/pico-sdk.git C:\pico-sdk
cd C:\pico-sdk
git submodule update --init
```

Set the environment variable (do this permanently in System Properties → Environment Variables):
```
PICO_SDK_PATH=C:\pico-sdk
```

**SDK 1.3.0 or newer is required.** The modules run as I2C slaves, which needs
the `pico_i2c_slave` library (`pico/i2c_slave.h`). It does not exist in 1.2.x.
Check with:
```
git -C C:\pico-sdk describe --tags
```

---

## 3. Build the Project

Open a new terminal (so PICO_SDK_PATH is visible), then:

```
cd "C:\Users\Jeff Houghten\Documents\Talker"
mkdir build
cd build
cmake .. -G "Ninja"
ninja
```

Each firmware target produces a `.uf2` file in its subfolder, e.g.:
```
build/central_controller/central_controller.uf2
build/modules/wires/module_wires.uf2
```

To build only one target:
```
ninja module_wires
```

---

## 4. Flash a Pico

1. Hold the **BOOTSEL** button on the Pico while plugging in USB.
2. It mounts as a USB drive (RPI-RP2).
3. Drag and drop the `.uf2` file onto the drive.
4. The Pico reboots and runs the new firmware.

---

## 5. Module Bus Wiring

All eleven Picos hang off one **I2C multi-drop bus** — three signals plus
ground, tapped in parallel at every module. There is no chain and no ordering;
unplugging a module in the middle does not cut off the ones after it.

```
      Central (I2C master)                                          3V3
      ┌──────────────┐                                               │
      │ GP2  SDA ────┼──┬────────┬────────┬────────┬─── ... ──┐    [4k7]
      │ GP3  SCL ────┼──┼──┬─────┼──┬─────┼──┬─────┼──        │      │
      │ GP6  ATTN ───┼──┼──┼──┬──┼──┼──┬──┼──┼──┬──┼──  ──┐   ├────[4k7]
      │ GND ─────────┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──  ──┼───┼────[4k7]
      └──────────────┘  │  │  │  │  │  │  │  │  │  │      │   │      │
                      ┌─┴──┴──┴─┐┌─┴──┴──┴─┐┌─┴──┴─┐   ┌──┴───┴──┐  │
                      │ Wires   ││ Keypad  ││ Simon │...│  Knob   │  │
                      │  0x11   ││  0x12   ││ 0x13  │   │  0x1A   │  │
                      └─────────┘└─────────┘└───────┘   └─────────┘  │
                                                                    GND
```

| Signal | GPIO | Physical Pin | Direction                                |
|--------|------|--------------|------------------------------------------|
| SDA    | GP2  | 4            | bidirectional (I2C1)                     |
| SCL    | GP3  | 5            | master drives (I2C1)                     |
| ATTN   | GP6  | 9            | modules pull LOW, central reads          |
| GND    | —    | 38           | common return, must be shared            |

**Pull-ups: three 4.7 kΩ resistors to 3V3, fitted once at the central end only.**
Do not add a pull-up per module — ten in parallel is 470 Ω, which overloads the
bus drivers. The firmware enables the Pico's internal pull-ups as a bench-test
convenience, but they are ~50 kΩ and far too weak for a real harness.

**Addresses** are `0x10 + module_id`, so Wires is 0x11 through Knob at 0x1A.
Each module's firmware sets its own address at compile time, so flash the right
`.uf2` to the right Pico.

### Why the ATTN line exists

An I2C slave cannot start a transfer, so a module with a strike to report has
no way to interrupt the central. Instead it queues the message and pulls the
shared ATTN line low. The central sees the line drop within one loop iteration
and sweeps the bus to collect it. ATTN is open-drain — modules only ever pull
down, never drive high — so any number can assert it at once.

ATTN is an optimisation, not a requirement. If you leave it unwired the central
falls back to polling one module every 200 ms and the game still works, just
with more latency on strikes. Wire it.

---

## 6. Standard Module Pinout

| GPIO | Physical Pin | Function              | Notes                              |
|------|--------------|-----------------------|------------------------------------|
| GP2  | 4            | I2C1 SDA              | **Inter-Pico bus** — do not reuse  |
| GP3  | 5            | I2C1 SCL              | **Inter-Pico bus** — do not reuse  |
| GP4  | 6            | I2C0 SDA              | This module's own displays         |
| GP5  | 7            | I2C0 SCL              |                                    |
| GP6  | 9            | ATTN (open-drain)     | **Inter-Pico bus** — do not reuse  |
| GP10 | 14           | RGB LED Red           | Status indicator (common cathode)  |
| GP11 | 15           | RGB LED Green         |                                    |
| GP12 | 16           | RGB LED Blue          |                                    |
| GP18 | 24           | SPI0 SCK              | SK9822 / addressable LED clock     |
| GP19 | 25           | SPI0 TX               | SK9822 / addressable LED data      |
| GP14+| 19+          | Module buttons/LEDs   | Varies by module                   |

GP0, GP1, GP8 and GP9 are now **free** — they were the old daisy-chain UARTs.

**Two separate I2C buses.** `i2c1` (GP2/GP3) is the inter-Pico bus and belongs
to the game. `i2c0` (GP4/GP5) is each module's private bus for its own OLEDs,
TCA9548A, or HT16K33. Never put a display on GP2/GP3 — a display address could
collide with a module address and the game would start talking to a screen.

**Note on the LED strip:** SK9822 SPI moved from GP2/GP3 to GP18/GP19 because
GP2/GP3 are now the bus. On the central controller the big-button ground pin
also moved from GP18 to GP21 to make room. Update `module_base.h`
`LED_STRIP_*_PIN` if you wire them differently.

---

## 7. Debugging

Enable USB serial in any `CMakeLists.txt` with:
```cmake
pico_enable_stdio_usb(target_name 1)
```
Then `printf()` output appears on the Pico's USB CDC serial port.
Use PuTTY, Tera Term, or `minicom` at any baud rate (USB CDC ignores baud).
