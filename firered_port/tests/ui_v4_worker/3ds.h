#ifndef HOST_UI_3DS_H
#define HOST_UI_3DS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef int32_t s32;typedef int64_t s64;typedef uint64_t u64;typedef int32_t Result;
typedef int32_t LightLock;
typedef struct HostThread *Thread;
#define CUR_THREAD_HANDLE 1
#define R_FAILED(r) ((r)<0)
void LightLock_Init(LightLock*);void LightLock_Lock(LightLock*);void LightLock_Unlock(LightLock*);
Result svcGetThreadPriority(s32*,unsigned);void svcSleepThread(s64);
Thread threadCreate(void(*)(void*),void*,size_t,int,int,bool);
Result threadJoin(Thread,u64);void threadFree(Thread);
#endif
