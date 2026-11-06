/* SPDX-License-Identifier: MIT */
#ifndef FRE_SERVICE_MANIFEST_H
#define FRE_SERVICE_MANIFEST_H
#include "update_core.h"
#define FRE_SERVICE_PATCHES 8
#define FRE_SERVICE_JSON_BYTES 16384
typedef struct {
    char from_version[32],asset[128];
    uint64_t source_bytes,patch_bytes;
    unsigned char source_sha[32],patch_sha[32];
} FreServicePatch;
typedef struct {
    char version[32];uint64_t game_bytes;unsigned char game_sha[32];
    unsigned patch_count;FreServicePatch patch[FRE_SERVICE_PATCHES];
} FreServiceManifest;
bool FreService_ParseManifest(const char *json,size_t bytes,FreServiceManifest *out);
#endif
