#ifndef FR_EMERALD_V4_SDK_H
#define FR_EMERALD_V4_SDK_H
#include <stdbool.h>
#include <stdint.h>
/* Fixed-width SDK boundary; no game structs or game ABI flags here. */
void FrEmeraldV4_SdkInit(void);
void FrEmeraldV4_LockInit(int32_t *lock);
void FrEmeraldV4_Lock(int32_t *lock);
void FrEmeraldV4_Unlock(int32_t *lock);
void FrEmeraldV4_ArenaLock(void);
void FrEmeraldV4_ArenaUnlock(void);
bool FrEmeraldV4_StartWorker(void (*entry)(void *),void *arg,unsigned stack,int core);
bool FrEmeraldV4_StopWorker(void);
bool FrEmeraldV4_WorkerStopping(void);
void FrEmeraldV4_SleepUs(unsigned microseconds);
#endif
