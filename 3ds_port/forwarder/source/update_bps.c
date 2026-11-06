/* SPDX-License-Identifier: MIT
 * BPS1 file-to-file decoder. TargetCopy supports overlap without loading the
 * game image. Every read/write and number is bounded before it is used. */
#include "update_core.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#define BLOCK 16384u
typedef struct{FILE *file;uint64_t at,end;} Reader;
static uint32_t Le32(const unsigned char *p){return(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static bool Read(Reader *r,void *data,size_t size)
{if(size>r->end-r->at||fread(data,1,size,r->file)!=size)return false;r->at+=size;return true;}
static bool Number(Reader *r,uint64_t *out)
{
    uint64_t value=0,shift=1;
    for(unsigned n=0;n<10;n++){
        unsigned char byte;if(!Read(r,&byte,1))return false;
        uint64_t digit=byte&127u;
        if(digit&&(shift>UINT64_MAX/digit||value>UINT64_MAX-digit*shift))return false;
        value+=digit*shift;
        if(byte&128u){*out=value;return true;}
        if(shift>UINT64_MAX/128u)return false;
        shift*=128u;if(value>UINT64_MAX-shift)return false;value+=shift;
    }
    return false;
}
static bool Seek(FILE *file,uint64_t at)
{return at<=(uint64_t)LONG_MAX&&fseek(file,(long)at,SEEK_SET)==0;}
static bool Offset(uint64_t previous,uint64_t encoded,uint64_t limit,uint64_t *out)
{
    uint64_t step=encoded>>1;
    if(encoded&1u){if(step>previous)return false;*out=previous-step;}
    else{if(step>limit||previous>limit-step)return false;*out=previous+step;}
    return *out<=limit;
}
static bool Write(FILE *target,const unsigned char *data,size_t size,FreSha256 *hash,uint32_t *crc)
{
    if(fwrite(data,1,size,target)!=size)return false;
    FreSha256_Update(hash,data,size);*crc=FreCrc32(*crc,data,size);return true;
}
bool FreBps_Apply(const char *source,const char *patch,const char *stage,
                  const unsigned char expected_source[32],const unsigned char expected_patch[32],
                  const unsigned char expected_target[32],uint64_t expected_bytes,
                  FreUpdateProgress progress,void *user,char error[160])
{
    FreFileInfo si,pi;FILE *sf=NULL,*pf=NULL,*tf=NULL;bool ok=false;
    unsigned char buffer[BLOCK],footer[12],hash_out[32];uint64_t source_size,target_size,metadata;
    uint64_t output=0,source_relative=0,target_relative=0;uint32_t target_crc=0;FreSha256 target_hash;
    if(error)error[0]=0;
#define FAIL(message) do{if(error)snprintf(error,160,"%s",message);goto done;}while(0)
    if(!source||!patch||!stage||!strcmp(source,stage)||!strcmp(patch,stage))FAIL("Invalid update file paths.");
    if(!expected_bytes||expected_bytes>FRE_UPDATE_MAX_BYTES)FAIL("Invalid updated game size.");
    if(progress)progress("Checking installed game",0,1,user);
    if(!FreHashFile(source,&si)||!FreHashEqual(si.sha256,expected_source))FAIL("Installed game does not match this update. Build the current version with the patcher.");
    if(progress)progress("Checking update data",0,1,user);
    /* CRC32 of data followed by its little-endian IEEE CRC is fixed. This
     * residue check includes the BPS source/target footer too. */
    if(!FreHashFile(patch,&pi)||pi.bytes<16||pi.crc32!=0x2144df1cu||!FreHashEqual(pi.sha256,expected_patch))FAIL("Update data verification failed. The installed game is unchanged.");
    sf=fopen(source,"rb");pf=fopen(patch,"rb");
    if(!sf||!pf)FAIL("Cannot open update files.");
    if(!Seek(pf,pi.bytes-12)||fread(footer,1,12,pf)!=12||!Seek(pf,0))FAIL("Cannot read update checksums.");
    if(Le32(footer)!=si.crc32)FAIL("Installed game checksum differs.");
    Reader reader={pf,0,pi.bytes-12};
    if(!Read(&reader,buffer,4)||memcmp(buffer,"BPS1",4)||!Number(&reader,&source_size)||!Number(&reader,&target_size)||!Number(&reader,&metadata))FAIL("Invalid BPS update header.");
    if(source_size!=si.bytes||target_size!=expected_bytes||metadata>65536u||metadata>reader.end-reader.at)FAIL("Update size or metadata bounds differ.");
    if(!Seek(pf,reader.at+metadata))FAIL("Invalid update metadata.");
    reader.at+=metadata;
    int fd=open(stage,O_CREAT|O_EXCL|O_RDWR,0666);
    if(fd<0)FAIL("A staged update already exists or the SD card is full. Open Recovery before retrying.");
    tf=fdopen(fd,"w+b");if(!tf){close(fd);FAIL("Cannot open the staged game.");}
    FreSha256_Init(&target_hash);
    while(output<target_size){
        uint64_t command,length,encoded,at=0;
        if(!Number(&reader,&command))FAIL("Truncated update command.");
        length=(command>>2)+1u;
        if(length>target_size-output)FAIL("Update command exceeds the game size.");
        unsigned kind=(unsigned)(command&3u);
        if(kind==0){at=output;if(at>source_size||length>source_size-at)FAIL("Update source read is outside the game.");}
        else if(kind==2){if(!Number(&reader,&encoded)||!Offset(source_relative,encoded,source_size,&at)||length>source_size-at)FAIL("Update source copy is outside the game.");source_relative=at+length;}
        else if(kind==3){if(!Number(&reader,&encoded)||!Offset(target_relative,encoded,target_size,&at)||at>=output)FAIL("Update target copy reads unwritten data.");target_relative=at+length;}
        while(length){
            size_t n=length>BLOCK?BLOCK:(size_t)length;
            if(kind==1){if(!Read(&reader,buffer,n))FAIL("Truncated update literal.");}
            else if(kind==3){
                uint64_t distance=output-at;size_t seed=distance<n?(size_t)distance:n;
                if(fflush(tf)||!Seek(tf,at)||fread(buffer,1,seed,tf)!=seed)FAIL("Cannot read staged update copy.");
                for(size_t i=seed;i<n;i++)buffer[i]=buffer[i-seed];
            }else if(!Seek(sf,at)||fread(buffer,1,n,sf)!=n)FAIL("Truncated installed game.");
            /* Ordinary output is already sequential. Only TargetCopy reads
             * from this update stream; its seek also supplies the required
             * C read-to-write transition even when the positions coincide. */
            if((kind==3&&!Seek(tf,output))||!Write(tf,buffer,n,&target_hash,&target_crc))FAIL("Cannot write updated game. Check free SD card space.");
            output+=n;at+=n;length-=n;
            if(progress)progress("Preparing updated game",output,target_size,user);
        }
    }
    if(reader.at!=reader.end)FAIL("Unexpected trailing update commands.");
    FreSha256_Finish(&target_hash,hash_out);
    if(target_crc!=Le32(footer+4)||!FreHashEqual(hash_out,expected_target))FAIL("Updated game verification failed. The installed game is unchanged.");
    if(fflush(tf))FAIL("Cannot finish writing the updated game.");
    ok=true;
done:
    if(sf&&fclose(sf))ok=false;
    if(pf&&fclose(pf))ok=false;
    if(tf&&fclose(tf))ok=false;
    if(!ok&&error&&!error[0])snprintf(error,160,"The update could not finish; the installed game is unchanged.");
    return ok;
#undef FAIL
}
