/* SPDX-License-Identifier: MIT
 * SHA-256 (FIPS180-4) and IEEE CRC32, bounded streaming file helpers. */
#include "update_core.h"
#include <stdio.h>
#include <string.h>
static uint32_t Ror(uint32_t x,unsigned n){return(x>>n)|(x<<(32-n));}
static uint32_t Be32(const unsigned char *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void PutBe(unsigned char *p,uint32_t x){p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x;}
static void Transform(FreSha256 *h,const unsigned char *data)
{
    static const uint32_t k[64]={
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t w[16],a=h->state[0],b=h->state[1],c=h->state[2],d=h->state[3];
    uint32_t e=h->state[4],f=h->state[5],g=h->state[6],z=h->state[7];
    for(unsigned i=0;i<16;i++)w[i]=Be32(data+4*i);
    for(unsigned i=0;i<64;i++){
        unsigned n=i&15;
        if(i>=16){uint32_t x=w[(i-15)&15],y=w[(i-2)&15];w[n]+=(Ror(x,7)^Ror(x,18)^(x>>3))+w[(i-7)&15]+(Ror(y,17)^Ror(y,19)^(y>>10));}
        uint32_t t1=z+(Ror(e,6)^Ror(e,11)^Ror(e,25))+((e&f)^(~e&g))+k[i]+w[n];
        uint32_t t2=(Ror(a,2)^Ror(a,13)^Ror(a,22))+((a&b)^(a&c)^(b&c));
        z=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    h->state[0]+=a;h->state[1]+=b;h->state[2]+=c;h->state[3]+=d;
    h->state[4]+=e;h->state[5]+=f;h->state[6]+=g;h->state[7]+=z;
}
void FreSha256_Init(FreSha256 *h)
{
    static const uint32_t initial[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    memcpy(h->state,initial,sizeof(initial));h->bytes=0;h->used=0;
}
void FreSha256_Update(FreSha256 *h,const void *bytes,size_t size)
{
    const unsigned char *data=bytes;h->bytes+=size;
    while(size){size_t n=64-h->used;if(n>size)n=size;memcpy(h->block+h->used,data,n);h->used+=(unsigned)n;data+=n;size-=n;if(h->used==64){Transform(h,h->block);h->used=0;}}
}
void FreSha256_Finish(FreSha256 *h,unsigned char output[32])
{
    uint64_t bits=h->bytes*8;h->block[h->used++]=0x80;
    if(h->used>56){memset(h->block+h->used,0,64-h->used);Transform(h,h->block);h->used=0;}
    memset(h->block+h->used,0,56-h->used);for(unsigned i=0;i<8;i++)h->block[63-i]=(unsigned char)(bits>>(8*i));Transform(h,h->block);
    for(unsigned i=0;i<8;i++)PutBe(output+4*i,h->state[i]);
    memset(h,0,sizeof(*h));
}
uint32_t FreCrc32(uint32_t previous,const void *bytes,size_t size)
{
    static const uint32_t table[16]={0,0x1db71064,0x3b6e20c8,0x26d930ac,0x76dc4190,0x6b6b51f4,0x4db26158,0x5005713c,0xedb88320,0xf00f9344,0xd6d6a3e8,0xcb61b38c,0x9b64c2b0,0x86d3d2d4,0xa00ae278,0xbdbdf21c};
    const unsigned char *data=bytes;uint32_t crc=~previous;
    while(size--){crc^=*data++;crc=(crc>>4)^table[crc&15];crc=(crc>>4)^table[crc&15];}
    return~crc;
}
bool FreHashEqual(const unsigned char a[32],const unsigned char b[32])
{unsigned diff=0;for(unsigned i=0;i<32;i++)diff|=a[i]^b[i];return diff==0;}
bool FreHashFile(const char *path,FreFileInfo *info)
{
    FILE *f=fopen(path,"rb");unsigned char block[16384];FreSha256 hash;size_t n;bool ok=true;
    if(!f)return false;
    info->bytes=0;info->crc32=0;FreSha256_Init(&hash);
    while((n=fread(block,1,sizeof(block),f))!=0){info->bytes+=n;if(info->bytes>FRE_UPDATE_MAX_BYTES){ok=false;break;}FreSha256_Update(&hash,block,n);info->crc32=FreCrc32(info->crc32,block,n);}
    if(ferror(f))ok=false;
    if(fclose(f))ok=false;
    FreSha256_Finish(&hash,info->sha256);return ok;
}
static int Hex(unsigned char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return-1;}
bool FreHashFromHex(const char *text,unsigned char hash[32])
{if(!text||strlen(text)!=64)return false;for(unsigned i=0;i<32;i++){int a=Hex(text[2*i]),b=Hex(text[2*i+1]);if(a<0||b<0)return false;hash[i]=(unsigned char)(a*16+b);}return true;}
void FreHashToHex(const unsigned char hash[32],char text[65])
{static const char hex[]="0123456789abcdef";for(unsigned i=0;i<32;i++){text[2*i]=hex[hash[i]>>4];text[2*i+1]=hex[hash[i]&15];}text[64]=0;}
