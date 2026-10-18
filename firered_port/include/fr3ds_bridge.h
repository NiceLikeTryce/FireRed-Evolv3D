/* SPDX-License-Identifier: MIT */
#ifndef FR3DS_BRIDGE_H
#define FR3DS_BRIDGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "3ds_video.h"
#include "3ds_input.h"
#define FR3DS_BRIDGE_ABI 1u
#define FR3DS_DEX_LIMIT 251u
#define FR3DS_FLASH_BYTES 0x20000u
/* Virtualised GBA hardware for native ARM11 game code, not a CPU emulator. */
typedef struct {
    uint8_t regs[0x400];
    uint8_t vram[0x18000];
    uint16_t palette[512];
    uint16_t oam[512];
    uint8_t ewram[0x40000];
    uint8_t iwram[0x8000];
} Fr3dsHardware;
extern Fr3dsHardware gFr3dsHardware;
void Fr3ds_ResetHardware(void);
CtrVideoMemory Fr3ds_VideoMemory(void);
uint16_t Fr3ds_UpdateInput(const CtrInputSample *sample);
/* Timing: 0 immediate,1 VBlank,2 HBlank. FIFO/audio DMA is not implemented. */
bool Fr3ds_DmaSet(unsigned channel, const void *source, void *dest, uint32_t control);
void Fr3ds_RunDma(unsigned timing);
void Fr3ds_CpuSet(const void *source, void *dest, uint32_t control);
void Fr3ds_CpuFastSet(const void *source, void *dest, uint32_t control);
/* FireRed Evolved keeps32-bit attributes even on imported Emerald layouts. */
unsigned Fr3ds_PrimaryCount(bool importedEmerald);
bool Fr3ds_MetatileBank(bool importedEmerald, unsigned id, unsigned *bank, unsigned *local);
uint32_t Fr3ds_ReadAttribute(const uint8_t *data, size_t bytes, unsigned index, bool *ok);
uint16_t Fr3ds_MapId(uint16_t block);
unsigned Fr3ds_MapCollision(uint16_t block);
unsigned Fr3ds_MapElevation(uint16_t block);
/* The port-check executable is diagnostic. Only a future full engine can set
 * playable=true; linking a few engine units must never advertise a game. */
typedef struct {
    const char *name;
    bool nativeCore, playable;
    unsigned dexLimit, taskCount;
    uint16_t randomValue;
} Fr3dsCoreInfo;
void Fr3ds_CoreInit(void);
void Fr3ds_CoreTick(uint16_t down);
Fr3dsCoreInfo Fr3ds_CoreInfo(void);
#endif
