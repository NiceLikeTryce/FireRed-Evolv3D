/* SPDX-License-Identifier: MIT */
#ifndef FRE_UPDATE_HTTP_H
#define FRE_UPDATE_HTTP_H
#include "update_core.h"
bool FreHttp_Init(char error[160]);
void FreHttp_Shutdown(void);
bool FreHttp_Buffer(const char *url,char *buffer,size_t capacity,size_t *bytes,char error[160]);
bool FreHttp_File(const char *url,const char *path,uint64_t size,const unsigned char sha[32],FreUpdateProgress progress,void *user,char error[160]);
#endif
