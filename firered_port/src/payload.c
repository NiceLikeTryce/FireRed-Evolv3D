/* SPDX-License-Identifier: MIT */
#include "fr3ds_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t Read32(const uint8_t *p) { uint32_t n; memcpy(&n, p, 4); return n; }
static bool ReadExact(const char *path, void *p, size_t bytes)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    bool ok = fread(p, 1, bytes, f) == bytes && fgetc(f) == EOF;
    fclose(f); return ok;
}
bool Fr3ds_LoadPayload(const char *dataPath, const char *relocPath, void *region,
                      uint32_t regionBytes, uint32_t referenceAddress)
{
    uint8_t header[24];
    FILE *f = fopen(relocPath, "rb");
    if (!f) return false;
    bool ok = fread(header, 1, 24, f) == 24;
    fclose(f);
    if (!ok || memcmp(header, "F3DP", 4) || Read32(header + 4) != 1 || Read32(header + 8) != regionBytes) return false;
    uint32_t internal = Read32(header + 16), external = Read32(header + 20);
    uint64_t bytes = 24ull + 4ull * ((uint64_t)internal + external);
    if (bytes > 16 * 1024 * 1024 || !region || regionBytes < 4) return false;
    uint8_t *rel = malloc((size_t)bytes);
    if (!rel) return false;
    ok = ReadExact(relocPath, rel, bytes) && ReadExact(dataPath, region, regionBytes);
    if (ok) for (uint32_t i = 0; i < internal + external; ++i) {
        uint32_t site = Read32(rel + 24 + i * 4);
        if (site > regionBytes - 4) { ok = false; break; }
        uint8_t *p = (uint8_t *)region + site;
        uint32_t value = Read32(p);
        if (i < internal && value >= regionBytes) { ok = false; break; }
        value += i < internal ? (uint32_t)(uintptr_t)region : referenceAddress - Read32(header + 12);
        memcpy(p, &value, 4);
    }
    free(rel);
    return ok;
}
