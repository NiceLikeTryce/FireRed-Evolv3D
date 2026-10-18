/* SPDX-License-Identifier: MIT */
#include "fr3ds_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static void put(uint8_t *p,uint32_t v){memcpy(p,&v,4);}
static uint32_t get(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void file(const char*path,const void*p,size_t n){FILE*f=fopen(path,"wb");assert(f);assert(fwrite(p,1,n,f)==n);assert(fclose(f)==0);}
int main(void){
 uint8_t data[20]={0},header[32]={0},region[20]={0};
 memcpy(header,"F3DP",4);put(header+4,1);put(header+8,20);put(header+12,0x1000);put(header+16,1);put(header+20,1);put(header+24,1);put(header+28,9);put(data+1,19);put(data+9,0x1020);
 file("payload.bin",data,20);file("payload.rel",header,32);
 assert(Fr3ds_LoadPayload("payload.bin","payload.rel",region,20,0x2000));
 assert(get(region+1)==(uint32_t)(uintptr_t)region+19&&get(region+9)==0x2020);
 put(header+24,17);file("payload.rel",header,32);assert(!Fr3ds_LoadPayload("payload.bin","payload.rel",region,20,0x2000));
 put(header+24,1);put(data+1,20);file("payload.bin",data,20);file("payload.rel",header,32);assert(!Fr3ds_LoadPayload("payload.bin","payload.rel",region,20,0x2000));
 put(data+1,19);file("payload.bin",data,19);assert(!Fr3ds_LoadPayload("payload.bin","payload.rel",region,20,0x2000));file("payload.bin",data,20);
 file("payload.rel",header,31);assert(!Fr3ds_LoadPayload("payload.bin","payload.rel",region,20,0x2000));
 put(header+16,0xffffffff);file("payload.rel",header,32);assert(!Fr3ds_LoadPayload("payload.bin","payload.rel",region,20,0x2000));put(header+16,1);
 header[0]='X';file("payload.rel",header,32);assert(!Fr3ds_LoadPayload("payload.bin","payload.rel",region,20,0x2000));header[0]='F';
 put(header+4,2);file("payload.rel",header,32);assert(!Fr3ds_LoadPayload("payload.bin","payload.rel",region,20,0x2000));put(header+4,1);
 file("payload.rel",header,32);assert(!Fr3ds_LoadPayload("payload.bin","payload.rel",region,19,0x2000));assert(!Fr3ds_LoadPayload("payload.bin","payload.rel",NULL,20,0x2000));assert(!Fr3ds_LoadPayload("absent.bin","payload.rel",region,20,0x2000));
 remove("payload.bin");remove("payload.rel");puts("PASS payload relocation: unaligned internal/external sites; malformed lengths, addresses, version, counts, files and region rejected");return 0;
}
