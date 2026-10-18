/* SPDX-License-Identifier: MIT */
#ifndef FR3DS_RENDER_H
#define FR3DS_RENDER_H
#include "3ds_video.h"
void Fr3ds_RenderText256(CtrVideoMemory memory, unsigned bg, uint16_t control,
                        unsigned hofs, unsigned vofs, unsigned y, uint16_t *line,
                        uint16_t *layerMask, const uint16_t *windowMask, bool blend);
#endif
