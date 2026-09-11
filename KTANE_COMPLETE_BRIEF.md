# KTANE Physical Bomb Project - Complete Brief for Claude Code

\---

# Keep Talking and Nobody Explodes - Physical Version

## Project Requirements Document

A physical, hardware-based implementation of the bomb defusal puzzle game "Keep Talking and Nobody Explodes." The system consists of multiple Raspberry Pi Pico microcontrollers communicating via UART serial connections, each managing dedicated game modules. A central controller manages game state, timing, level progression, strikes, and module unlock/difficulty mechanics.

\---

## Hardware Architecture

### Central Controller (Main Pico)

* **Role**: Game state management, timing, level tracking, strike counting, Big Button module
* **Responsibilities**:

  * Maintains bomb timer and countdown
  * Tracks number of strikes
  * Manages current level and difficulty settings
  * Handles game state transitions (active, paused, won, lost)
  * Controls module unlock/lock states
  * Manages level progression and unlock conditions
  * Handles Big Button input

* **Hardware Specs**:
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 1 RGB LED connected to pins 9 - Blue, 10 - Green, 11 - Red and 8 - GND on the Pico
  - TCA9458A I2C multiplexer connected to pins 6 - SDA, 7 - SCL on Pico
  - 1.2" 7-Segment LED HT16K33 Backpack to display the timer during the game and additional info pre-game connected to pins SD0 and SC0 on the I2C multiplexer
  - SSD1306 OLED display 128x64 to display 0, 1, or 2 batteries connected to pins SD1 and SC1 on the I2C multiplexer
  - SSD1306 OLED display 128x64 to display 0, 1 or 2 batteries connected to pins SD2 and SC2 on the I2C multiplexer
  - SSD1306 OLED display 128x32 to display the serial number connected to pins SD3 and SC3 on the I2C multiplexer
  - SSD1306 OLED display 128x64 to display Number of strikes during the game and additional info pre-game connected to pins SD4 and SC4 on the I2C multiplexer
  - SSD1306 OLED display 128x64 to display the text for the Big Button connected to pins SD6 and SC6 on the I2C multiplexer
  - SSD1306 OLED display 128x32 to display a 3 digit indicator connected to pins SD7 and SC7 on the I2C multiplexer
  - 12x WS2812 RGB LED Strip for the color of the Big Button connected to Pin 15 on the Pico
  - 11x SK9822 RGB LED strip for the color of the strip for the Big Button module connected to pins 16 and 17 on the Pico
  - 1 momentary button connected to pin 20 and pin 18 on the Pico

### Module Picos (10 additional Picos)

Each module runs independently on its own Pico and communicates with the central controller via UART.

**Module List**:
1. **Wires** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 6 wires with unique resistors on each wire connected to pin 31 - A0, pin 36 - 3.3V, and pin 38, GND on the pico
  - 1 RGB LED connected to pins 9 - Blue, 10 - Green, 11 - Red and 8 - GND on the Pico
  ]
2. **Big Button** - Hardware Specs: [Part of main controller]
3. **Keypad** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 1 RGB LED connected to pins 9 - Blue, 10 - Green, 11 - Red and 8 - GND on the Pico
  - TCA9458A I2C multiplexer connected to pins 6 - SDA, 7 - SCL on Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD0 and SC0 on the I2C multiplexer
  - momentary button connected to pin 21 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD1 and SC1 on the I2C multiplexer
  - momentary button connected to pin 22 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD2 and SC2 on the I2C multiplexer
  - momentary button connected to pin 24 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD3 and SC3 on the I2C multiplexer
  - momentary button connected to pin 25 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD4 and SC4 on the I2C multiplexer
  - momentary button connected to pin 26 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD5 and SC5 on the I2C multiplexer
  - momentary button connected to pin 27 on the Pico
  ]
4. **Simon Says** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 1 RGB LED connected to pins 9 - Blue, 10 - Green, 11 - Red and 8 - GND on the Pico
  - Red LED connected to pin 14 on the Pico
  - momentary button connected to pin 24 on the Pico
  - Blue LED connected to pin 15 on the Pico
  - momentary button connected to pin 25 on the Pico
  - Green LED connected to pin 16 on the Pico
  - momentary button connected to pin 26 on the Pico
  - Yellow LED connected to pin 17 on the Pico
  - momentary button connected to pin 27 on the Pico
  ]
5. **Who's on First** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 1 RGB LED connected to pins 9 - Blue, 10 - Green, 11 - Red and 8 - GND on the Pico
  - TCA9458A I2C multiplexer connected to pins 6 - SDA, 7 - SCL on Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD0 and SC0 on the I2C multiplexer
  - momentary button connected to pin 21 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD1 and SC1 on the I2C multiplexer
  - momentary button connected to pin 22 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD2 and SC2 on the I2C multiplexer
  - momentary button connected to pin 24 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD3 and SC3 on the I2C multiplexer
  - momentary button connected to pin 25 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD4 and SC4 on the I2C multiplexer
  - momentary button connected to pin 26 on the Pico
  - SSD1306 OLED display 128x64 to display a symbol connected to pins SD5 and SC5 on the I2C multiplexer
  - momentary button connected to pin 27 on the Pico
  - SSD1306 OLED display 128x32 to display a symbol connected to pins SD7 and SC7 on the I2C multiplexer
  - 5x SK9822 RGB LED strip for the number of times to repeat connected to pins 16 and 17 on the Pico  
  ]
6. **Memory** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 1 RGB LED connected to pins 9 - Blue, 10 - Green, 11 - Red and 8 - GND on the Pico
  - 5x 7 Segment displays (need to figure out how to connect)
  - 4x momentary button connected to pins 21-25 on the Pico
  - 5x SK9822 RGB LED strip for the number of times to repeat connected to pins 16 and 17 on the Pico
  ]
7. **Switches** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 1 RGB LED connected to pins 9 - Blue, 10 - Green, 11 - Red and 8 - GND on the Pico
  - 5x SPDT switches connected to the Pico (need to figure out how to connect)
  - 10x Red LED connected to the Pico (need to figure out how to connect possibly use a SK9822 RGB LED strip)
  ]
8. **Maze** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 1 RGB LED connected to pins 9 - Blue, 10 - Green, 11 - Red and 8 - GND on the Pico
  - 36x SK9822 RGB LED strip connected to pins 16 and 17 on the Pico
  - 4x momentary buttons connected to pins 24, 25, 26, 27 on the Pico
  ]
9. **Password** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 1 RGB LED connected to pins 9 - Blue, 10 - Green, 11 - Red and 8 - GND on the Pico
  - TCA9458A I2C multiplexer connected to pins 6 - SDA, 7 - SCL on Pico
  - SSD1306 OLED display 128x32 to display a symbol connected to pins SD0 and SC0 on the I2C multiplexer
  - 2x momentary buttons connected to pins 14, 15 on the Pico
  - SSD1306 OLED display 128x32 to display a symbol connected to pins SD1 and SC1 on the I2C multiplexer
  - 2x momentary buttons connected to pins 16, 17 on the Pico
  - SSD1306 OLED display 128x32 to display a symbol connected to pins SD2 and SC2 on the I2C multiplexer
  - 2x momentary buttons connected to pins 19, 20 on the Pico
  - SSD1306 OLED display 128x32 to display a symbol connected to pins SD3 and SC3 on the I2C multiplexer
  - 2x momentary buttons connected to pins 21, 22 on the Pico
  - SSD1306 OLED display 128x32 to display a symbol connected to pins SD4 and SC4 on the I2C multiplexer
  - 2x momentary buttons connected to pins 24, 25 on the Pico
  - momentary button for the submit button on pin 26 of the Pico
  ]
10. **Venting Gas** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - SSD1306 OLED display 128x32 connected to pins 6 - SDA, 7 - SCL on Pico
  - SSD1306 OLED display 128x32 to display a symbol connected to pins SD0 and SC0 on the I2C multiplexer
  - momentary button for the Yes button on pin 14 of the Pico
  - momentary button for the No button on pin 15 of the Pico
  - 2x 7 Segment displays connected to the Pico (need to figure out how to connect)
  ]
11. **Knob** - Hardware Specs: [
  - Raspberry Pi Pico
  - Pico pins 1 and 2 used for UART communication
  - 12x Red LED connected to the Pico (need to figure out how to connect possibly use a SK9822 RGB LED strip)
  - 2x 7 Segment displays connected to the Pico (need to figure out how to connect)
  - 4 position knob connected to the Pico (need to figure out how to connect)
  - An "Up" indicator (need to figure out what this is in reality and how to present it, maybe just 4 white LEDs that say "UP" and only 1 turns on?)
  ]


### Physical Layout

* **Front Panel**: Big Button, Wires, Keypad, Simon Says, Who's on First, Memory
* **Back Panel**: Switches (on/off for back modules), Maze, Password, Venting Gas, Knob
* **Back Panel Unlock**: Physical key mechanism triggered after completing front panel

\---

## Core Gameplay Mechanics

### Game Flow

1. Player selects level (if unlocked) or starts at Level 1
2. Central controller initializes all modules based on difficulty settings
3. Timer starts; players must defuse modules before time runs out
4. Each module solved correctly earns points and unlocks next round (if applicable)
5. Incorrect solution or timer expiration = strike
6. 3 strikes = bomb explodes (game over)
7. All modules solved = bomb defused (victory)

### Difficulty Progression

**Level Structure**:

|Level|Time (sec)|Strikes|Front Modules|Back Modules|Repeat Modules|Notes|
|-|-|-|-|-|-|-|
|1|\[TBD]|3|3-4|Locked|None|Unlock condition: \[TBD]|
|2|\[TBD]|3|4-5|Locked|None||
|3|\[TBD]|2|5-6|Locked|1-2||
|4|\[TBD]|2|All|Unlocked|2-3|Back panel switch active|
|5+|\[TBD]|1-2|All|All|2-3|Maze 8x8, harder logic|

\---

## Feature Requirements

### Level System

* Level selection screen (only unlocked levels available)
* Progressive difficulty curves
* Unlock mechanism: Complete front panel → key appears → unlock back panel
* Unlock tracking: Save completed levels/achievements
* Reset level: Return to Level 1 from any screen

### Time \& Strike Management

* Countdown timer displayed on main controller
* Strike counter increments on module failure
* Game over on 3 strikes (configurable per level)
* Game over on timer reaching zero

### Module Repeat Mechanics

* Who's on First: Multiple rounds with escalating difficulty
* Memory: Multi-round sequence with longer patterns
* Other modules: Configurable repeat count or single-solve per game

### Back Panel Control

* Physical switch on back panel: Enable/disable back modules during gameplay
* Back modules initially locked until front panel complete
* Back panel difficulty adjustable via level settings

### Module-Specific Enhancements

* **All Modules**: Clear solved/unsolved visual indicator
* **All Modules**: UART protocol for communication with central controller

\---

## Communication Protocol

> **SUPERSEDED.** This section describes the original UART plan. The build now
> uses an I2C multi-drop bus (i2c1 on GP2/GP3, plus a shared ATTN line on GP6)
> with the central as master and each module as a slave at `0x10 + module_id`.
> Every other mention of UART in this brief is historical. The authoritative
> spec is `shared/bus_protocol.h`; wiring is in `SETUP.md` §5 and
> `SYSTEM_DIAGRAM.md` §2.

### UART Specifications (original plan, no longer implemented)

* **Baud Rate**: \[TBD]
* **Message Structure**: `\[Header]\[Module\_ID]\[Message\_Type]\[Payload]\[Checksum]`
* **Message Types**: Status updates, solution submission, configuration, timer signals

\---

## Deliverables

**Code**: Central controller, module templates, 11 module firmware, UART library, level config
**Documentation**: Hardware assembly, pinouts, UART protocol, module guide, troubleshooting
**Configuration**: Level definitions, difficulty presets, unlock rules

\---

## Open Questions

* Back panel unlock: Physical key, electronic, or time-based?
* Module repeats: Fixed count or difficulty-based?
* Central display: LCD, OLED, or 7-segment?

\---

\---

# EXISTING CODE CONTEXT

**Instructions for Claude Code:**

This is work-in-progress code from the physical KTANE bomb defusal project. The code is incomplete and has bugs, but contains useful patterns and logic. Please:

* **Review the architecture** and reuse patterns that make sense
* **Fix bugs** (there are several typos and incomplete implementations)
* **Refactor heavily** if the overall design doesn't align with the multi-Pico UART architecture
* **Discard and rebuild** any sections that don't fit the requirements
* **Ask clarifying questions** if intent is unclear

The existing code is **Arduino-based** and focuses on the **central controller** (Big Button module + main game state). It uses I2C multiplexing (TCA9548A) to manage multiple OLED displays and a 7-segment timer.

\---

## Existing Files Overview

### GameState.h

Central game state tracker with:

* Enum for game states (IDLE, START, TIMER, WIN, LOSE)
* Level, strike, and module tracking
* Flag system for tracking game events
* Module completion status

**Status**: Core structure is sound; needs integration with level definitions and UART comms.

### ModuleState.h / ModuleState.cpp

Big Button module logic. Contains:

* Color flag definitions for button LEDs
* Big Button rule engine (Immediate Release, detection of FRK indicator, battery counts)
* ROSC-based random number generation
* Screen shuffling

**Status**: Logic is complex and working, but scattered. Needs organization into proper Big Button module firmware. Has bugs (see notes below).

### IOManager.h / IOManager.cpp

Hardware I/O abstraction for displays and LEDs. Contains:

* I2C multiplexer (TCA9548A) control for 6 OLED displays
* 7-segment timer display management
* RGB button LED color control
* Text/graphics rendering methods

**Status**: Mostly complete for central controller; has typos and incomplete implementations.

### KeepTalking.ino

Main sketch file (referenced but full content not critical for review)

\---

## Known Issues \& Bugs

1. **IOManager.cpp line 58**: `tcaselect()` should be `tcaSelect()` (case mismatch)
2. **IOManager.cpp line 35**: `tcaselect()` should be `tcaSelect()` (multiple instances)
3. **IOManager.cpp `setButtonColor()`**: Only writes to RED pin; should write R, G, B separately
4. **ModuleState.cpp `shuffleScreens()`**: Parameter missing parens in function signature
5. **ModuleState.cpp**: Missing function body for `setButtonColor()` (only declared in .h)
6. **IOManager.h**: Hardcoded screen array; should be configurable
7. **IOManager.cpp `writeText()`**: Function stub with no implementation
8. **ModuleState.cpp Big Button logic**: Deeply nested, repetitive code; could be table-driven

\---

## Useful Patterns to Keep

* **Flag-based state tracking** (GameState): Clean approach for bitmask operations
* **I2C multiplexer abstraction** (IOManager.tcaSelect): Good separation of hardware detail
* **ROSC-based RNG** (ModuleState): Proper Pico-specific entropy source
* **Message type enum** (IOManager.MessageType): Clear API for display variants

\---

## Architecture Recommendations

The current code is **central-controller-only**. For the full multi-Pico system, we'll need:

1. **Central Controller Firmware** (`main\_controller.ino` or `.cpp`)

   * Keep GameState, IOManager
   * Add UART communication layer
   * Integrate level definitions from requirements
2. **Module Base Firmware** (`module\_base.h/cpp`)

   * UART message parsing/sending
   * State machine for module lifecycle
   * Common utilities for all modules
3. **Big Button Module Firmware** (`bigbutton\_module.ino`)

   * Extract and refactor ModuleState logic
   * Use module base for UART comms
   * Simplify rule engine (table-driven approach)
4. **UART Communication Library** (`uart\_protocol.h/cpp`)

   * Message structure: `\[Header]\[Module\_ID]\[Message\_Type]\[Payload]\[Checksum]`
   * Request/response handling
   * Timeout and error management

\---

## Existing Code Files (Below)

\[See full code content in sections that follow]

\---

## GameState.h

```cpp
#ifndef GAME\_STATE\_H
#define GAME\_STATE\_H

#include <Arduino.h>
#include "Adafruit\_LEDBackpack.h"

// Module flags (bit positions)
#define BIGBUTTON 0x01
#define WIRES 0x02
#define SIMON 0x04
// Add more modules here

// Game flags
#define FLAG\_GAME\_START 0x01
#define FLAG\_MODULE\_COMPLETED 0x02

enum GameStateEnum {
  STATE\_IDLE,
  STATE\_START,
  STATE\_TIMER,
  STATE\_WIN,
  STATE\_LOSE
};

class GameState {
public:
  GameStateEnum currentState;

  unsigned long startTime;
  unsigned long gameDuration;

  uint8\_t level;
  uint8\_t strikes;
  uint8\_t requiredModules;
  uint8\_t disarmedModules;
  uint16\_t gameFlags;  // Changed to 16-bit to accommodate more flags

  GameState();

  // State management
  void setState(GameStateEnum newState);
  bool isState(GameStateEnum state) const { return currentState == state; }

  // Time management
  unsigned long getRemainingTime() const;
  bool isTimeUp() const;

  // Module management
  void setModuleRequired(uint8\_t moduleBit) { requiredModules |= moduleBit; }
  void setModuleDisarmed(uint8\_t moduleBit) { disarmedModules |= moduleBit; }
  bool isModuleDisarmed(uint8\_t moduleBit) const { return (disarmedModules \& moduleBit) != 0; }
  bool allModulesDisarmed() const { return (disarmedModules \& requiredModules) == requiredModules; }

  // Flag management
  void setFlag(uint16\_t flag) { gameFlags |= flag; }
  void clearFlag(uint16\_t flag) { gameFlags \&= \~flag; }
  bool hasFlag(uint16\_t flag) const { return (gameFlags \& flag) != 0; }

  // Strike management
  void addStrike();
  uint8\_t getStrikes() const { return strikes; }

  // Reset for new game
  void reset();

  // Level management
  void loadLevel();
  void incrementLevel();
};

#endif // GAME\_STATE\_H
```

\---

## ModuleState.h

```cpp
#ifndef MODULE\_STATE\_H
#define MODULE\_STATE\_H

// Big Button specific flags
#define COLOR\_RED 0x01
#define COLOR\_BLUE 0x02
#define COLOR\_YELLOW 0x04
#define COLOR\_WHITE 0x08
#define DETONATE 0x10
#define HOLD 0x20
#define FRK\_IND 0x40
#define BAT\_COUNT\_2 0x80
#define BAT\_COUNT\_3 0x100

class ModuleState {
private:
  uint32\_t seed\_random\_from\_rosc();

public:
  void shuffleScreens(uint8\_t NUM\_SCREENS);
  void setButtonColor(uint8\_t r, uint8\_t g, uint8\_t b);
};
#endif // MODULE\_STATE\_H
```

\---

## IOManager.h

```cpp
#ifndef IO\_MANAGER\_H
#define IO\_MANAGER\_H

#include <Wire.h>
#include <Adafruit\_GFX.h>
#include <Adafruit\_SSD1306.h>

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans24pt7b.h>

const int NUM\_SCREENS = 3;
int screens\[NUM\_SCREENS] = { 1, 4, 6 };

const uint8\_t BUTTON\_PIN = 21;

#define SCREEN\_WIDTH 128
#define SCREEN\_HEIGHT\_L 64
#define SCREEN\_HEIGHT\_S 32

#define TCAADDR 0x70
#define SCREENADDR 0x3C
#define SEVENSEGADDR 0x71

enum MessageType {
  MSG\_NORMAL = 0,
  MSG\_BUTTON\_TEXT = 1,
  MSG\_INDICATOR = 2,
  MSG\_ONE\_AA = 3,
  MSG\_TWO\_AA = 4,
  MSG\_ONE\_D = 5
};

const uint8\_t NUM\_COLORS = 7;
const uint8\_t COLORS\[NUM\_COLORS]\[3] = {
  { 255, 0, 0 },     // Red
  { 0, 255, 0 },     // Green
  { 0, 0, 255 },     // Blue
  { 255, 255, 0 },   // Yellow
  { 0, 255, 255 },   // Cyan
  { 255, 0, 255 },   // Magenta
  { 255, 255, 255 }  // White
};

const uint8\_t NUM\_BUTTONTEXT = 5;
const char\* BUTTONTEXT\[] = {
  "Press",
  "",
  "Abort",
  "Detonate",
  "Hold"
};

const uint8\_t NUM\_INDICATORS = 12;
const char\* INDICATORS\[] = {
  "",
  "SND",
  "CLR",
  "IND",
  "FRQ",
  "SIG",
  "NSA",
  "MSA",
  "TRN",
  "BOB",
  "CAR",
  "FRK"
};

class IOManager {
private:
  Adafruit\_SSD1306 displayL;
  Adafruit\_SSD1306 displayS;
  Adafruit\_7segment matrix;

  void tcaSelect(uint8\_t channel);

  Adafruit\_SSD1306\& selectDisplay(uint8\_t channel);

public:
  IOManager();

  void begin();
  void clearAll();
  void clearDisplay(uint8\_t displayNum);

  // Text display methods
  void writeButtonText(uint8\_t displayNum, const char\* text);
  void writeIndicator(uint8\_t displayNum, const char\* text);

  // Battery display methods
  void drawBatteryOneAA(uint8\_t displayNum);
  void drawBatteryTwoAA(uint8\_t displayNum);
  void drawBatteryOneD(uint8\_t displayNum);

  // Timer display methods
  void updateTimer(unsigned long millisRemaining);
  void clearTimer();
  void showTimerMessage(const char\* msg);

  // RGB Button LED
  void setButtonColor(uint8\_t r, uint8\_t g, uint8\_t b);
};

#endif // IO\_MANAGER\_H
```

\---

## IOManager.cpp

```cpp
#include "IOManager.h"

static const uint8\_t BUTTON\_B\_LED\_PIN = 20;
static const uint8\_t BUTTON\_G\_LED\_PIN = 19;
static const uint8\_t BUTTON\_R\_LED\_PIN = 18;

static const uint8\_t LARGE\_CHANNELS\[] = { 1, 2, 4, 5, 6 };

static const uint8\_t ALL\_CHANNELS\[] = { 1, 2, 3, 4, 6, 7 };

IOManager::IOManager()
  : displayL(SCREEN\_WIDTH, SCREEN\_HEIGHT\_L, \&Wire, -1),
    displayS(SCREEN\_WIDTH, SCREEN\_HEIGHT\_S, \&Wire, -1) {
}

void IOManager::tcaSelect(uint8\_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

Adafruit\_SSD1306\& IOManager::selectDisplay(uint8\_t channel) {
  for (uint8\_t i = 0; i < sizeof(LARGE\_CHANNELS); i++) {
    if (LARGE\_CHANNELS\[i] == channel) return displayL;
  }
  return displayS;
}

void IOManager::begin() {
  Wire.begin();

  pinMode(BUTTON\_PIN, INPUT\_PULLUP);
  pinMode(BUTTON\_R\_LED\_PIN, OUTPUT);
  pinMode(BUTTON\_G\_LED\_PIN, OUTPUT);
  pinMode(BUTTON\_B\_LED\_PIN, OUTPUT);

  // Setup the timer display.
  tcaSelect(0);
  matrix.begin(SEVENSEGADDR);

  for (uint8\_t i = 0; i < sizeof(ALL\_CHANNELS); i++) {
    uint8\_t ch = ALL\_CHANNELS\[i];
    tcaSelect(ch);
    Adafruit\_SSD1306\& disp = selectDisplay(ch);
    if (!disp.begin(SSD1306\_SWITCHCAPVCC, SCREENADDR)) {
      Serial.print("SSD1306 init failed ch ");
      Serial.println(ch);
    }
    disp.clearDisplay();
    disp.display();
  }

  matrix.begin(SEVENSEGADDR);
  matrix.clear();
  matrix.writeDisplay();

  pinMode(BUTTON\_LED\_R\_PIN, OUTPUT);
  pinMode(BUTTON\_LED\_G\_PIN, OUTPUT);
  pinMode(BUTTON\_LED\_B\_PIN, OUTPUT);
  setButtonColor(0, 0, 0);
}

void IOManager::clearAll() {
  for (uint8\_t i = 0; i < sizeof(ALL\_CHANNELS); i++) {
    clearDisplay(ALL\_CHANNELS\[i]);
  }
  clearTimer();
}

void IOManager::clearDisplay(uint8\_t displayNum) {
  tcaSelect(displayNum);
  Adafruit\_SSD1306\& disp = selectDisplay(displayNum);
  disp.clearDisplay();
  disp.display();
}

void IOManager::writeText(uint8\_t displayNum, const char\* text, MessageType type) {
  // TODO: Implement based on MessageType
}

void IOManager::writeButtonText(uint8\_t displayNum, const char\* text) {
  tcaSelect(displayNum);
  displayL.clearDisplay();
  displayL.setFont(\&FreeSans12pt7b);
  displayL.setTextColor(WHITE);
  displayL.setCursor((SCREEN\_WIDTH / 2) - 50, (SCREEN\_HEIGHT\_L / 2) + 6);
  displayL.println(text);
  displayL.display();
}

void IOManager::writeIndicator(uint8\_t displayNum, const char\* text) {
  tcaSelect(displayNum);
  displayL.clearDisplay();
  displayL.setFont(\&FreeSans24pt7b);
  displayL.setTextColor(WHITE);
  displayL.setCursor((SCREEN\_WIDTH / 2) - 50, SCREEN\_HEIGHT\_L - 5);
  displayL.println(text);
  displayL.display();
}

void IOManager::drawBatteryOneAA(uint8\_t displayNum) {
  tcaSelect(displayNum);
  displayL.clearDisplay();
  displayL.drawRoundRect(5, 0, SCREEN\_WIDTH - 5, 30, 6, 1);
  displayL.drawRect(0, 5, 6, 20, 1);
  displayL.drawLine(25, 1, 25, 29, 1);
  displayL.display();
}

void IOManager::drawBatteryTwoAA(uint8\_t displayNum) {
  tcaSelect(displayNum);
  displayL.clearDisplay();
  displayL.drawRoundRect(5, 0, SCREEN\_WIDTH - 5, 30, 6, 1);
  displayL.drawRoundRect(0, 32, SCREEN\_WIDTH - 5, 30, 6, 1);
  displayL.drawRect(0, 5, 6, 20, 1);
  displayL.drawRect(SCREEN\_WIDTH - 6, 37, 6, 20, 1);
  displayL.drawLine(25, 1, 25, 29, 1);
  displayL.drawLine(SCREEN\_WIDTH - 25, 33, SCREEN\_WIDTH - 25, 61, 1);
  displayL.display();
}

void IOManager::drawBatteryOneD(uint8\_t displayNum) {
  tcaSelect(displayNum);
  displayL.clearDisplay();
  displayL.drawRoundRect(5, 0, SCREEN\_WIDTH - 10, SCREEN\_HEIGHT\_L - 2, 6, 1);
  displayL.drawRect(0, 21, 6, 20, 1);
  displayL.drawLine(25, 1, 25, SCREEN\_HEIGHT\_L - 3, 1);
  displayL.display();
}

void IOManager::updateTimer(unsigned long millisRemaining) {
  if (millisRemaining >= 60000) {
    unsigned long minutes = millisRemaining / 1000 / 60;
    unsigned long seconds = (millisRemaining / 1000) % 60;
    int displayValue = minutes \* 100 + seconds;
    matrix.print(displayValue, DEC);
    if (minutes < 10) {
      matrix.writeDigitNum(0, 0);
      if (minutes < 1) {
        matrix.writeDigitNum(1, 0);
      }
    }
    if (seconds < 10) {
      matrix.writeDigitNum(2, 0);
    }
    matrix.drawColon(seconds % 2 == 0);
    matrix.writeDisplay();
  } else {
    int displayValue = millisRemaining / 10;
    matrix.print(displayValue, DEC);
    if (displayValue < 1000) {
      matrix.writeDigitNum(0, 0);
      if (displayValue < 100) {
        matrix.writeDigitNum(1, 0);
        if (displayValue < 10) {
          matrix.writeDigitNum(2, 0);
          if (displayValue < 1) {
            matrix.writeDigitNum(3, 0);
          }
        }
      }
    }
    matrix.writeDisplay();
  }
}

void IOManager::clearTimer() {
  // TODO: Implement
}

void IOManager::showTimerMessage(const char\* msg) {
  matrix.clear();
  matrix.writeDisplay();
  matrix.print(msg);
  matrix.writeDisplay();
}

void IOManager::setButtonColor(uint8\_t r, uint8\_t g, uint8\_t b) {
  analogWrite(BUTTON\_R\_LED\_PIN, r);
  analogWrite(BUTTON\_G\_LED\_PIN, g);
  analogWrite(BUTTON\_B\_LED\_PIN, b);
}
```

\---

## ModuleState.cpp

```cpp
#include "ModuleState.h"

uint32\_t ModuleState::seed\_random\_from\_rosc() {
  uint32\_t random = 0x811c9dc5;
  uint8\_t next\_byte = 0;
  volatile uint32\_t\* rnd\_reg = (uint32\_t\*)(ROSC\_BASE + ROSC\_RANDOMBIT\_OFFSET);

  for (uint8\_t i = 0; i < 16; i++) {
    for (uint8\_t k = 0; k < 8; k++) {
      next\_byte = (next\_byte << 1) | (\*rnd\_reg \& 1);
    }
    random ^= next\_byte;
    random \*= 0x01000193;
  }
  return random;
}

void ModuleState::shuffleScreens(uint8\_t numScreens) {
  // Assumes external 'screens' array exists
  for (int i = numScreens - 1; i > 0; i--) {
    uint32\_t randomVal = seed\_random\_from\_rosc();
    int j = randomVal % numScreens;
    // Swap screens\[i] and screens\[j]
    int temp = screens\[i];
    screens\[i] = screens\[j];
    screens\[j] = temp;
  }
}

void ModuleState::setButtonColor(uint8\_t r, uint8\_t g, uint8\_t b) {
  // Big Button specific implementation
  // To be integrated with IOManager once architecture is finalized
}

// Big Button rule engine (simplified excerpt)
// Full implementation involves complex nested logic for determining
// Immediate Release conditions, hold/detonate rules based on:
// - Button color
// - Button text (Press, Abort, Detonate, Hold)
// - Indicator presence (FRK)
// - Battery count
// See ModuleState.cpp full code for complete logic
```

\---

## Next Steps for Claude Code

1. **Review the bugs and architecture** above
2. **Outline the multi-Pico firmware structure** and confirm design approach
3. **Create the UART protocol library** (`uart\_protocol.h/cpp`)
4. **Scaffold the module base template** (`module\_base.h/cpp`)
5. **Refactor central controller** with fixes and UART integration
6. **Extract Big Button logic** into proper module firmware
7. **Plan the remaining 10 modules** with common patterns

What should we prioritize first?

