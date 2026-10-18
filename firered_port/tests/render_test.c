/* SPDX-License-Identifier: MIT */
#include "fr3ds_bridge.h"
#include "fr3ds_render.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    Fr3ds_ResetHardware();
    CtrVideoMemory m = Fr3ds_VideoMemory();
    uint16_t line[240], mask[240], win[240];
    uint16_t *regs = (uint16_t *)gFr3dsHardware.regs;
    for (unsigned i=0;i<256;i++) gFr3dsHardware.palette[i]=i;
    for (unsigned y=0;y<8;y++) for(unsigned x=0;x<8;x++) gFr3dsHardware.vram[64+y*8+x]=1+x+8*y;
    for(unsigned i=0;i<4096;i++) {gFr3dsHardware.vram[0x8000+i*2]=1;gFr3dsHardware.vram[0x8001+i*2]=0xf0;}
    for(unsigned size=0;size<4;size++) for(unsigned y=0;y<160;y++) {
        memset(line,0,sizeof(line));memset(mask,0,sizeof(mask));
        Fr3ds_RenderText256(m,0,0x1080|(size<<14),17,25,y,line,mask,NULL,false);
        for(unsigned x=0;x<240;x++) assert(line[x]==(0x8000|(1+((x+17)&7)+8*((y+25)&7))));
    }
    for(unsigned i=0;i<4096;i++) gFr3dsHardware.vram[0x8001+i*2]=0xfc;
    Fr3ds_RenderText256(m,0,0x1080,0,0,2,line,mask,NULL,false);
    for(unsigned x=0;x<240;x++) assert(line[x]==(0x8000|(1+7-(x&7)+8*5)));
    for(unsigned i=0;i<240;i++){line[i]=0x1234;mask[i]=2;win[i]=i&1?0:1;}
    Fr3ds_RenderText256(m,0,0x1080,0,0,2,line,mask,win,false);
    for(unsigned x=1;x<240;x+=2) assert(line[x]==0x1234&&mask[x]==2);
    memset(gFr3dsHardware.vram+64,1,64);gFr3dsHardware.palette[1]=31;
    for(unsigned i=0;i<240;i++){line[i]=31<<10;mask[i]=2;win[i]=63;}
    regs[0x50/2]=(2<<8)|(1<<6);regs[0x52/2]=0x1f1f;
    Fr3ds_RenderText256(m,0,0x1080,0,0,0,line,mask,win,true);
    assert(line[0]==(0x8000|0x7c1f));
    regs[0x50/2]=2<<6;regs[0x54/2]=31;
    Fr3ds_RenderText256(m,0,0x1080,0,0,0,line,mask,win,true);assert(line[0]==0xffff);
    regs[0x50/2]=3<<6;
    Fr3ds_RenderText256(m,0,0x1080,0,0,0,line,mask,win,true);assert(line[0]==0x8000);
    win[0]=1;
    Fr3ds_RenderText256(m,0,0x1080,0,0,0,line,mask,win,true);assert(line[0]==0x801f);
    puts("PASS native 256-color text: four map sizes, scroll, 153600 pixels, palette-bank ignore, both flips, window clipping/effects, blend/brightness saturation");
}
