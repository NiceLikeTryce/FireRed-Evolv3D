/* SPDX-License-Identifier: MIT */
#include "fr3ds_bridge.h"
#include <string.h>
Fr3dsHardware gFr3dsHardware __attribute__((aligned(4)));
static CtrInput sInput;
typedef struct { uintptr_t source,dest,initialDest; uint32_t control,count; } Dma;
static Dma sDma[4];
static void reg16(unsigned offset,uint16_t value) {
    gFr3dsHardware.regs[offset]=(uint8_t)value;
    gFr3dsHardware.regs[offset+1]=(uint8_t)(value>>8);
}
static void reg32(unsigned offset,uint32_t value) { reg16(offset,(uint16_t)value);reg16(offset+2,(uint16_t)(value>>16)); }
static uint16_t read16(unsigned offset) {
    return gFr3dsHardware.regs[offset] | ((uint16_t)gFr3dsHardware.regs[offset+1]<<8);
}
void Fr3ds_ResetHardware(void) {
    memset(&gFr3dsHardware,0,sizeof(gFr3dsHardware));
    memset(&sInput,0,sizeof(sInput)); memset(sDma,0,sizeof(sDma));
    reg16(0x130,0x3ff); reg16(0x208,1);
}
CtrVideoMemory Fr3ds_VideoMemory(void) {
    CtrVideoMemory memory={gFr3dsHardware.vram,gFr3dsHardware.palette,
                          gFr3dsHardware.oam,(const uint16_t *)gFr3dsHardware.regs};
    return memory;
}
uint16_t Fr3ds_UpdateInput(const CtrInputSample *sample) {
    if (!sample) return sInput.held;
    CtrInput_Update(&sInput,sample);
    uint16_t held=sInput.held;
    if(sInput.physicalHeld&CTR_KEY_Y) held|=CTR_KEY_SELECT;
    reg16(0x130,(held^0x3ff)&0x3ff);
    return held;
}
static void transfer(unsigned channel) {
    Dma *d=&sDma[channel]; unsigned width=d->control&0x04000000u?4:2;
    unsigned srcMode=(d->control>>23)&3,dstMode=(d->control>>21)&3;
    uintptr_t src=d->source&~(uintptr_t)(width-1),dst=d->dest&~(uintptr_t)(width-1);
    for(uint32_t i=0;i<d->count;i++) {
        uint32_t value=0; memcpy(&value,(const void *)src,width); memcpy((void *)dst,&value,width);
        if(srcMode<2) src+=srcMode==1?-(intptr_t)width:(intptr_t)width;
        if(dstMode!=2) dst+=dstMode==1?-(intptr_t)width:(intptr_t)width;
    }
    d->source=src;d->dest=dst;
    unsigned timing=(d->control>>28)&3;
    if((d->control&0x02000000u)&&timing) {
        if(dstMode==3)d->dest=d->initialDest;
    } else {
        d->control&=~0x80000000u;reg16(0xba+12*channel,(uint16_t)(d->control>>16));
    }
}
bool Fr3ds_DmaSet(unsigned channel,const void *source,void *dest,uint32_t control) {
    if(channel>=4)return false;
    if(!(control&0x80000000u)){sDma[channel].control=control;reg16(0xba+12*channel,(uint16_t)(control>>16));return true;}
    if(!source||!dest||((control>>28)&3)==3||((control>>23)&3)==3)return false;
    uint32_t count=control&(channel==3?0xffffu:0x3fffu);
    sDma[channel]=(Dma){(uintptr_t)source,(uintptr_t)dest,(uintptr_t)dest,control,count?count:(channel==3?0x10000u:0x4000u)};
    reg32(0xb0+12*channel,(uint32_t)(uintptr_t)source);
    reg32(0xb4+12*channel,(uint32_t)(uintptr_t)dest);
    reg32(0xb8+12*channel,control);
    if(!((control>>28)&3))transfer(channel);
    return true;
}
void Fr3ds_RunDma(unsigned timing) {
    if(timing>2)return;
    for(unsigned i=0;i<4;i++) {
        if(!(read16(0xba+12*i)&0x8000))continue;
        if((sDma[i].control&0x80000000u)&&((sDma[i].control>>28)&3)==timing)transfer(i);
    }
}
void Fr3ds_CpuSet(const void *source,void *dest,uint32_t control) {
    unsigned width=control&0x04000000u?4:2;uint32_t count=control&0x1fffffu;
    if(!source||!dest)return;
    uintptr_t s=(uintptr_t)source&~(uintptr_t)(width-1),d=(uintptr_t)dest&~(uintptr_t)(width-1);
    for(uint32_t i=0;i<count;i++){uint32_t value=0;memcpy(&value,(const void *)s,width);memcpy((void *)d,&value,width);d+=width;if(!(control&0x01000000u))s+=width;}
}
void Fr3ds_CpuFastSet(const void *source,void *dest,uint32_t control) {
    uint32_t words=((control&0x1fffffu)+7)&~7u;
    if(!source||!dest)return;
    uintptr_t s=(uintptr_t)source&~(uintptr_t)3,d=(uintptr_t)dest&~(uintptr_t)3;
    for(uint32_t i=0;i<words;i++){uint32_t value;memcpy(&value,(const void *)s,4);memcpy((void *)d,&value,4);d+=4;if(!(control&0x01000000u))s+=4;}
}
unsigned Fr3ds_PrimaryCount(bool importedEmerald){return importedEmerald?512u:640u;}
bool Fr3ds_MetatileBank(bool importedEmerald,unsigned id,unsigned *bank,unsigned *local) {
    if(id>=1024||!bank||!local)return false;
    unsigned split=Fr3ds_PrimaryCount(importedEmerald);*bank=id>=split;*local=id>=split?id-split:id;return true;
}
uint32_t Fr3ds_ReadAttribute(const uint8_t *data,size_t bytes,unsigned index,bool *ok) {
    size_t offset=(size_t)index*4;
    bool valid=data&&offset/4==index&&offset<=bytes&&bytes-offset>=4;
    if(ok)*ok=valid;
    if(!valid)return 0;
    return (uint32_t)data[offset]|((uint32_t)data[offset+1]<<8)|((uint32_t)data[offset+2]<<16)|((uint32_t)data[offset+3]<<24);
}
uint16_t Fr3ds_MapId(uint16_t block){return block&1023;}
unsigned Fr3ds_MapCollision(uint16_t block){return(block>>10)&3;}
unsigned Fr3ds_MapElevation(uint16_t block){return block>>12;}
