/* SPDX-License-Identifier: MIT */
#include "fr3ds_bridge.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    Fr3ds_ResetHardware();CtrVideoMemory v=Fr3ds_VideoMemory();
    assert(v.vram==gFr3dsHardware.vram&&v.palette==gFr3dsHardware.palette&&v.oam==gFr3dsHardware.oam);
    assert(v.regs[0x130/2]==0x3ff);CtrInputSample input={0};input.buttons=CTR_KEY_A|CTR_KEY_Y;assert(Fr3ds_UpdateInput(&input)==(CTR_KEY_A|CTR_KEY_SELECT));assert(v.regs[0x130/2]==(0x3ff^(CTR_KEY_A|CTR_KEY_SELECT)));
    input.buttons=0;input.circleY=80;assert(Fr3ds_UpdateInput(&input)==CTR_KEY_UP);input.buttons=CTR_KEY_LEFT;assert(Fr3ds_UpdateInput(&input)==CTR_KEY_LEFT);
    uint32_t source[]={0x10203040,0x12345678,0x87654321,0xaabbccdd},dest[32]={0};
    assert(Fr3ds_DmaSet(3,source,dest,0x84000004));assert(!memcmp(source,dest,16));
    memset(dest,0,sizeof(dest));assert(Fr3ds_DmaSet(1,source,dest,0x85000004));for(unsigned i=0;i<4;i++)assert(dest[i]==source[0]);
    memset(dest,0,sizeof(dest));assert(Fr3ds_DmaSet(2,source,dest,0x94000004));assert(!dest[0]);Fr3ds_RunDma(2);assert(!dest[0]);Fr3ds_RunDma(1);assert(!memcmp(source,dest,16));
    memset(dest,0,sizeof(dest));assert(Fr3ds_DmaSet(0,source,dest,0xa6600001));Fr3ds_RunDma(2);assert(dest[0]==source[0]);Fr3ds_RunDma(2);assert(dest[0]==source[1]);
    assert(Fr3ds_DmaSet(0,NULL,NULL,0));Fr3ds_RunDma(2);assert(dest[0]==source[1]);
    assert(!Fr3ds_DmaSet(4,source,dest,0x84000001));assert(!Fr3ds_DmaSet(3,NULL,dest,0x84000001));assert(!Fr3ds_DmaSet(3,source,dest,0xb4000001));
    memset(dest,0,sizeof(dest));Fr3ds_CpuSet(source,dest,0x04000004);assert(!memcmp(source,dest,16));Fr3ds_CpuFastSet(source,dest,0x01000001);for(unsigned i=0;i<8;i++)assert(dest[i]==source[0]);
    for(unsigned mode=0;mode<2;mode++)for(unsigned id=0;id<1024;id++){unsigned bank,local,split=mode?512:640;assert(Fr3ds_MetatileBank(mode,id,&bank,&local));assert(bank==(id>=split)&&local==(id>=split?id-split:id));}
    unsigned bank,local;assert(!Fr3ds_MetatileBank(false,1024,&bank,&local));
    uint8_t attrs[]={0x12,0x34,0x56,0x78,0x01,0x02,0x03,0x04};bool ok;assert(Fr3ds_ReadAttribute(attrs,8,1,&ok)==0x04030201&&ok);assert(!Fr3ds_ReadAttribute(attrs,7,1,&ok)&&!ok);assert(!Fr3ds_ReadAttribute(NULL,0,0,&ok)&&!ok);
    for(unsigned block=0;block<65536;block++){assert(Fr3ds_MapId(block)==(block&1023));assert(Fr3ds_MapCollision(block)==((block>>10)&3));assert(Fr3ds_MapElevation(block)==block/4096);}
    assert(CtrVideo_RGBA5551(0x001f)==0xf801);assert(CtrVideo_RGBA5551(0x7c00)==0x003f);
    puts("PASS FireRed bridge: input, virtual video, immediate/fill/deferred/repeating DMA, BIOS copy,2048 bank cases,32-bit attributes,65536 map blocks");return 0;
}
