/* Current Emerald3DS lock/thread primitives, adapted to an owned joinable
 * UI worker. Join before FireRed restores game BSS or the UI arena pointers.
 * Source reference:3ds_platform.c at bcae4b7; versioned licence retained. */
#include <3ds.h>
#include "field_v4_sdk.h"
_Static_assert(sizeof(LightLock)==sizeof(int32_t),"native UI lock ABI");
static LightLock sArenaLock;
static Thread sWorker;
static uint32_t sStopping=1;
static bool sInitialized;

void FrEmeraldV4_SdkInit(void)
{
    /* Main-thread setup precedes any worker start or shared-arena allocation. */
    if(sInitialized)return;
    LightLock_Init(&sArenaLock);
    __atomic_store_n(&sStopping,1,__ATOMIC_RELEASE);
    sInitialized=true;
}
void FrEmeraldV4_LockInit(int32_t *p){LightLock_Init((LightLock *)p);}
void FrEmeraldV4_Lock(int32_t *p){LightLock_Lock((LightLock *)p);}
void FrEmeraldV4_Unlock(int32_t *p){LightLock_Unlock((LightLock *)p);}
void FrEmeraldV4_ArenaLock(void){FrEmeraldV4_SdkInit();LightLock_Lock(&sArenaLock);}
void FrEmeraldV4_ArenaUnlock(void){LightLock_Unlock(&sArenaLock);}
bool FrEmeraldV4_WorkerStopping(void)
{return __atomic_load_n(&sStopping,__ATOMIC_ACQUIRE)!=0;}
void FrEmeraldV4_SleepUs(unsigned us){svcSleepThread((s64)us*1000);}

bool FrEmeraldV4_StartWorker(void (*entry)(void *),void *arg,unsigned stack,int core)
{
    s32 priority=0x30;
    FrEmeraldV4_SdkInit();
    if(!entry || sWorker)return false; /* Never create duplicate UI workers. */
    svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    priority=priority<0x3f?priority+1:0x3f;
    __atomic_store_n(&sStopping,0,__ATOMIC_RELEASE);
    /* PlateDiskOpen requests the application's default core(-2). */
    sWorker=threadCreate(entry,arg,stack,priority,core,false);
    if(!sWorker)__atomic_store_n(&sStopping,1,__ATOMIC_RELEASE);
    return sWorker!=NULL;
}
bool FrEmeraldV4_StopWorker(void)
{
    __atomic_store_n(&sStopping,1,__ATOMIC_RELEASE);
    if(!sWorker)return true;
    /* Failure retains the owned handle and live game context for another join;
     * the caller must not restore BSS or reset its allocator in that case. */
    if(R_FAILED(threadJoin(sWorker,2000000000ULL)))return false;
    threadFree(sWorker);sWorker=NULL;
    return true;
}
