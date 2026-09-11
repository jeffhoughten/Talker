#include "save.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Flash layout
// The Pico has 2 MB of flash mapped at XIP base 0x10000000.
// We reserve the very last 4 KB sector for save data.
// ---------------------------------------------------------------------------
#define FLASH_SIZE_BYTES    (2 * 1024 * 1024)         // 2 MB
#define SAVE_SECTOR_OFFSET  (FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)  // 0x1FF000

// XIP-mapped read address (what the CPU sees for reads)
#define SAVE_XIP_ADDR       (XIP_BASE + SAVE_SECTOR_OFFSET)

// We only write one 256-byte page within the sector
#define SAVE_PAGE_OFFSET    0   // first page of the sector

// Magic number to detect valid save data ("KT" = Keep Talking)
#define SAVE_MAGIC          ((uint16_t)0x4B54)

// ---------------------------------------------------------------------------
// On-flash page layout (must fit in 256 bytes)
// ---------------------------------------------------------------------------
typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  current_level;
    uint8_t  highest_level_unlocked;
    uint16_t checksum;          // simple sum of the 4 data bytes above
    uint8_t  _pad[250];         // fill to 256 bytes
} SavePage;

_Static_assert(sizeof(SavePage) == FLASH_PAGE_SIZE, "SavePage must be 256 bytes");

// ---------------------------------------------------------------------------
// Checksum (just a sum — good enough for detecting flash corruption)
// ---------------------------------------------------------------------------
static uint16_t compute_checksum(const SavePage *p) {
    return (uint16_t)(p->magic
                    + p->current_level
                    + p->highest_level_unlocked);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void save_load(SaveData *out) {
    const SavePage *p = (const SavePage *)SAVE_XIP_ADDR;

    if (p->magic == SAVE_MAGIC && p->checksum == compute_checksum(p)) {
        out->current_level          = p->current_level;
        out->highest_level_unlocked = p->highest_level_unlocked;
    } else {
        // Blank flash (0xFF bytes) or corrupt — start fresh
        out->current_level          = 0;
        out->highest_level_unlocked = 0;
    }
}

void save_write(const SaveData *data) {
    SavePage page;
    memset(&page, 0xFF, sizeof(page));   // start with erased-flash value

    page.magic                  = SAVE_MAGIC;
    page.current_level          = data->current_level;
    page.highest_level_unlocked = data->highest_level_unlocked;
    page.checksum               = compute_checksum(&page);

    // Flash writes must not be interrupted and cannot run from XIP flash.
    // The SDK handles the RAM-trampoline requirement internally.
    uint32_t irq_state = save_and_disable_interrupts();

    flash_range_erase(SAVE_SECTOR_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SAVE_SECTOR_OFFSET + SAVE_PAGE_OFFSET,
                        (const uint8_t *)&page, FLASH_PAGE_SIZE);

    restore_interrupts(irq_state);
}

void save_erase(void) {
    uint32_t irq_state = save_and_disable_interrupts();
    flash_range_erase(SAVE_SECTOR_OFFSET, FLASH_SECTOR_SIZE);
    restore_interrupts(irq_state);
}
