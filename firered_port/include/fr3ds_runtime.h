/* SPDX-License-Identifier: MIT */
#ifndef FR3DS_RUNTIME_H
#define FR3DS_RUNTIME_H
#include <stdint.h>
#include <stdbool.h>
bool Fr3ds_RuntimeInit(void);
bool Fr3ds_RuntimeFrame(const uint16_t *pixels);
void Fr3ds_RuntimeShutdown(void);
uint16_t Fr3ds_RuntimeKeys(void);
void Fr3ds_RuntimeAudio(const int32_t *samples, uint32_t stereoFrames);
void Fr3ds_RuntimeResetAudio(void);
void Fr3ds_RuntimeLog(const char *message);
bool Fr3ds_LoadPayload(const char *dataPath, const char *relocPath, void *region,
                      uint32_t regionBytes, uint32_t referenceAddress);
#endif
