/* SPDX-License-Identifier: MIT */
#include "fr3ds_render.h"
static uint16_t U16(const uint8_t *p) { return p[0] | (p[1] << 8); }
static unsigned Min16(unsigned n) { return n > 16 ? 16 : n; }
void Fr3ds_RenderText256(CtrVideoMemory m, unsigned bg, uint16_t control,
                        unsigned hofs, unsigned vofs, unsigned y, uint16_t *line,
                        uint16_t *mask, const uint16_t *win, bool blend)
{
    unsigned mosaic = m.regs[0x4c / 2], mode = (m.regs[0x50 / 2] >> 6) & 3;
    unsigned size = control >> 14, mx = (mosaic & 15) + 1, my = ((mosaic >> 4) & 15) + 1;
    if (control & 0x40) y -= y % my;
    unsigned sy = (y + vofs) & ((size & 2) ? 511 : 255);
    for (unsigned x = 0; x < 240; ++x) {
        if (win && !(win[x] & (1u << bg))) continue;
        unsigned sx = ((control & 0x40 ? x - x % mx : x) + hofs) & ((size & 1) ? 511 : 255);
        unsigned map = ((control >> 8) & 31) * 2048 + CtrVideo_TextMapOffset(sx / 8, sy / 8, size);
        if (map + 1 >= 0x10000) continue;
        uint16_t entry = U16(m.vram + map);
        unsigned px = sx & 7, py = sy & 7;
        if (entry & 0x400) px = 7 - px;
        if (entry & 0x800) py = 7 - py;
        unsigned tile = ((control >> 2) & 3) * 16384 + (entry & 1023) * 64 + py * 8 + px;
        if (tile >= 0x10000) continue;
        unsigned index = m.vram[tile];
        if (!index) continue;
        uint16_t color = m.palette[index];
        if (blend && (!win || (win[x] & 32))) {
            unsigned result = 0;
            for (unsigned shift = 0; shift <= 10; shift += 5) {
                unsigned c = (color >> shift) & 31;
                if (mode == 1 && (mask[x] & (m.regs[0x50 / 2] >> 8))) {
                    c = (c * Min16(m.regs[0x52 / 2] & 31) + ((line[x] >> shift) & 31) * Min16((m.regs[0x52 / 2] >> 8) & 31)) >> 4;
                    if (c > 31) c = 31;
                } else if (mode == 2) c += (31 - c) * Min16(m.regs[0x54 / 2] & 31) / 16;
                else if (mode == 3) c -= c * Min16(m.regs[0x54 / 2] & 31) / 16;
                result |= c << shift;
            }
            color = result;
        }
        line[x] = color | 0x8000;
        mask[x] = 1u << bg;
    }
}
