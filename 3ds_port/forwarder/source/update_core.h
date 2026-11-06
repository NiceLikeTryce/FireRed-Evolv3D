/* SPDX-License-Identifier: MIT */
#ifndef FRE_UPDATE_CORE_H
#define FRE_UPDATE_CORE_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define FRE_UPDATE_MAX_BYTES (192u*1024u*1024u)
typedef struct {
    uint32_t state[8];uint64_t bytes;unsigned used;unsigned char block[64];
} FreSha256;
typedef struct {uint64_t bytes;unsigned char sha256[32];uint32_t crc32;} FreFileInfo;
typedef void (*FreUpdateProgress)(const char *phase,uint64_t done,uint64_t total,void *user);
void FreSha256_Init(FreSha256 *hash);
void FreSha256_Update(FreSha256 *hash,const void *data,size_t bytes);
void FreSha256_Finish(FreSha256 *hash,unsigned char output[32]);
uint32_t FreCrc32(uint32_t previous,const void *data,size_t bytes);
bool FreHashFile(const char *path,FreFileInfo *info);
bool FreHashEqual(const unsigned char a[32],const unsigned char b[32]);
bool FreHashFromHex(const char *hex,unsigned char hash[32]);
void FreHashToHex(const unsigned char hash[32],char hex[65]);
/* Source and patch are read-only. The owned stage must not exist. */
bool FreBps_Apply(const char *source,const char *patch,const char *stage,
                  const unsigned char expected_source[32],
                  const unsigned char expected_patch[32],
                  const unsigned char expected_target[32],uint64_t target_bytes,
                  FreUpdateProgress progress,void *user,char error[160]);
/* root is the application directory; save_root is left unchanged. */
bool FreUpdate_Install(const char *root,const char *save_root,const char *patch,
                       const unsigned char expected_source[32],
                       const unsigned char expected_patch[32],
                       const unsigned char expected_target[32],uint64_t target_bytes,
                       FreUpdateProgress progress,void *user,char error[160]);
bool FreUpdate_Recover(const char *root,char error[160]);
/* Explicit previous-version recovery also preserves the current game and saves. */
bool FreUpdate_Rollback(const char *root,const char *save_root,char error[160]);
#endif
