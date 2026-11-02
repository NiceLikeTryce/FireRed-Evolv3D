#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include "3ds.h"
#include "field_v4_sdk.h"
struct HostThread {pthread_t native;void(*entry)(void*);void *arg;};
static unsigned created,freed,joins,failCreate,failJoin;
static unsigned started;
static unsigned long arenaCounter;
void LightLock_Init(LightLock*p){__atomic_store_n(p,1,__ATOMIC_RELEASE);}
void LightLock_Lock(LightLock*p){for(;;){int32_t expected=1;if(__atomic_compare_exchange_n(p,&expected,0,false,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;sched_yield();}}
void LightLock_Unlock(LightLock*p){__atomic_store_n(p,1,__ATOMIC_RELEASE);}
Result svcGetThreadPriority(s32*p,unsigned h){assert(h==CUR_THREAD_HANDLE);*p=0x30;return 0;}
void svcSleepThread(s64 ns){assert(ns>=0);struct timespec t={ns/1000000000,ns%1000000000};nanosleep(&t,NULL);}
static void *enter(void *p){Thread t=p;t->entry(t->arg);return NULL;}
Thread threadCreate(void(*entry)(void*),void*arg,size_t stack,int priority,int core,bool detached)
{
 assert(!detached&&stack==16384&&priority==0x31&&core==-2);
 if(failCreate){--failCreate;return NULL;}
 Thread t=calloc(1,sizeof(*t));assert(t);t->entry=entry;t->arg=arg;
 assert(pthread_create(&t->native,NULL,enter,t)==0);++created;return t;
}
Result threadJoin(Thread t,u64 timeout)
{
 assert(t&&timeout==2000000000ULL);++joins;
 if(failJoin){--failJoin;return -1;}
 return pthread_join(t->native,NULL)==0?0:-1;
}
void threadFree(Thread t){assert(t);++freed;free(t);}
static void worker(void *arg)
{
 (void)arg;__atomic_add_fetch(&started,1,__ATOMIC_RELEASE);
 while(!FrEmeraldV4_WorkerStopping())FrEmeraldV4_SleepUs(20);
}
static void countWorker(void *arg)
{
 unsigned n=*(unsigned*)arg;
 for(unsigned i=0;i<n;++i){FrEmeraldV4_ArenaLock();++arenaCounter;FrEmeraldV4_ArenaUnlock();}
}
int main(void)
{
 FrEmeraldV4_SdkInit();FrEmeraldV4_SdkInit();
 assert(FrEmeraldV4_WorkerStopping()&&FrEmeraldV4_StopWorker());
 assert(!FrEmeraldV4_StartWorker(NULL,NULL,16384,-2));
 failCreate=1;assert(!FrEmeraldV4_StartWorker(worker,NULL,16384,-2));assert(FrEmeraldV4_WorkerStopping());
 assert(FrEmeraldV4_StartWorker(worker,NULL,16384,-2));
 assert(!FrEmeraldV4_StartWorker(worker,NULL,16384,-2));
 unsigned before=freed;failJoin=1;assert(!FrEmeraldV4_StopWorker());
 assert(freed==before&&FrEmeraldV4_WorkerStopping());
 assert(!FrEmeraldV4_StartWorker(worker,NULL,16384,-2)&&FrEmeraldV4_WorkerStopping());
 assert(FrEmeraldV4_StopWorker()&&freed==before+1);assert(FrEmeraldV4_StopWorker());
 for(unsigned i=0;i<64;++i){assert(FrEmeraldV4_StartWorker(worker,NULL,16384,-2));assert(FrEmeraldV4_StopWorker());}
 unsigned count=30000;assert(FrEmeraldV4_StartWorker(countWorker,&count,16384,-2));
 for(unsigned i=0;i<count;++i){FrEmeraldV4_ArenaLock();++arenaCounter;FrEmeraldV4_ArenaUnlock();}
 assert(FrEmeraldV4_StopWorker()&&arenaCounter==2ul*count);
 assert(created==66&&freed==created&&joins==created+1);
 puts("PASS owned UI worker: duplicate/start failure, retained handle on failed join, bounded join,64restarts,60000serialized arena operations, exact free ownership");
 return 0;
}
