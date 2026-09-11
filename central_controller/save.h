#pragma once

#include "pico/stdlib.h"

// ---------------------------------------------------------------------------
// Persistent save data — stored in the last flash sector (4 KB at the top
// of the 2 MB flash, address 0x101FF000).
//
// Layout of the 256-byte save page:
//   [0]     magic byte  0xKT (0x4B54) low byte — detects uninitialised flash
//   [1]     magic byte  high byte
//   [2]     current_level  (0-based index into LEVELS[])
//   [3]     highest_level_unlocked
//   [4..5]  checksum (sum of bytes 0-3, little-endian uint16_t)
//   [6..255] reserved / padding
// ---------------------------------------------------------------------------

typedef struct {
    uint8_t  current_level;           // Last level played
    uint8_t  highest_level_unlocked;  // Furthest level the player has reached
} SaveData;

// Load from flash into *out.  If flash is blank or corrupt, resets to defaults.
void save_load(SaveData *out);

// Write *data to flash.  Erases the sector first (takes ~50 ms).
void save_write(const SaveData *data);

// Erase save sector — resets to factory defaults on next load.
void save_erase(void);
