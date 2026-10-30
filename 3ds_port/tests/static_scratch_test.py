from pathlib import Path
import argparse
parser=argparse.ArgumentParser(description='Extract actual static mesh scratch functions for host failure/cleanup checks.')
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();e=args.output;e.mkdir(parents=True,exist_ok=True)
s=(Path(__file__).resolve().parents[1]/'src/voxel/ctr_voxel.c').read_text()
def function(signature):
    a=s.index(signature+'\n{');i=s.index('{',a);depth=1;j=i+1
    while depth:depth+=(s[j]=='{')-(s[j]=='}');j+=1
    return s[a:j]
a=s.index('static struct\n{\n    bool active, lit;\n    unsigned pageModel;');b=s.index('} sStaticBuild;',a)+len('} sStaticBuild;');state=s[a:b]
head=r'''#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
typedef struct {float u,v;short x,y,z,shade;} VoxelGpuVertex;
typedef struct {float x,y,z,u,v,shade;} VoxelVertex;
#define VOXEL_STATIC_BAD_BITS 8192u
#define VOXEL_STATIC_RETRY_FRAMES 120u
#define CTR_LOG_VIDEO 0
static unsigned sourceCount=15000,normalCalls,normalFailAt,linearCalls,normalFrees,linearFrees;
static bool sourceValid=true,linearFail;
static void *liveNormal[3],*liveLinear;
static void *ordinaryAlloc(size_t n){unsigned i=normalCalls++;assert(i<3);if(normalFailAt==i+1||normalFailAt==4)return NULL;return liveNormal[i]=malloc(n);}
static void ordinaryFree(void *p){if(!p)return;for(unsigned i=0;i<3;i++)if(liveNormal[i]==p){liveNormal[i]=NULL;normalFrees++;free(p);return;}assert(!"Invalid or double ordinary free");}
static void *linearAlloc(size_t n){linearCalls++;assert(!liveLinear);if(linearFail)return NULL;return liveLinear=malloc(n);}
static void linearFree(void *p){assert(p&&p==liveLinear);free(p);liveLinear=NULL;linearFrees++;}
static bool VoxelBuildings_StaticModel(unsigned model,const VoxelVertex **src,unsigned *count,float *ox,float *oy,float *su,float *sv){static VoxelVertex vertices[3];assert(model==17);*src=vertices;*count=sourceCount;*ox=*oy=0;*su=*sv=1;return sourceValid;}
static void CtrLog_Write(int category,const char *fmt,...){(void)category;(void)fmt;}
static uint32_t sStaticBad[VOXEL_STATIC_BAD_BITS/32u],sFrame=100,sStaticRetryFrame;
static unsigned sStaticRetryModel=~0u;
static void *sStaticLinearScratch;
static bool sStaticLinearScratchLogged;
#define malloc ordinaryAlloc
#define free ordinaryFree
'''
body=state+'\n'+function('static void StaticBuildEnd(bool bad)')+'\n'+function('static bool StaticBuildStart(unsigned pageModel, bool lit)')
main=r'''
static void reset(unsigned failure,bool failLinear,unsigned count,bool valid){
 assert(!liveLinear&&!sStaticLinearScratch);for(unsigned i=0;i<3;i++)assert(!liveNormal[i]);
 normalCalls=linearCalls=normalFrees=linearFrees=0;normalFailAt=failure;linearFail=failLinear;sourceCount=count;sourceValid=valid;sStaticRetryModel=~0u;sStaticRetryFrame=0;memset(sStaticBad,0,sizeof(sStaticBad));
}
int main(void){unsigned tests=0;assert(sizeof(VoxelGpuVertex)==16);
 for(unsigned countIndex=0;countIndex<3;countIndex++)for(unsigned failure=0;failure<=4;failure++)for(unsigned failLinear=0;failLinear<2;failLinear++){
  unsigned count=(unsigned[]){3,15000,196605}[countIndex];reset(failure,failLinear,count,true);
  bool expected=failure==0||!failLinear,ok=StaticBuildStart(17,false);assert(ok==expected);
  assert(normalCalls==3&&linearCalls==(failure?1u:0u));
  if(ok){assert(sStaticBuild.active&&sStaticBuild.count==count&&sStaticBuild.packed&&sStaticBuild.index&&sStaticBuild.table);assert(((uintptr_t)sStaticBuild.table&3u)==0);assert(sStaticBuild.table[0]==-1&&sStaticBuild.table[sStaticBuild.buckets-1]==-1);
   if(failure){assert(sStaticLinearScratch==liveLinear);assert((uint8_t*)sStaticBuild.packed==(uint8_t*)liveLinear);assert((uint8_t*)sStaticBuild.index==(uint8_t*)liveLinear+count*sizeof(VoxelGpuVertex));assert((uint8_t*)sStaticBuild.table>=(uint8_t*)sStaticBuild.index+count*sizeof(uint16_t));}
   StaticBuildEnd(false);
  }else{assert(!sStaticBuild.active&&sStaticRetryModel==17&&sStaticRetryFrame==sFrame+120);}
  assert(!liveLinear&&!sStaticLinearScratch);for(unsigned i=0;i<3;i++)assert(!liveNormal[i]);
  assert(linearFrees==(failure&&!failLinear?1u:0u));tests++;
 }
 for(unsigned bad=0;bad<3;bad++){reset(0,false,bad==0?2:bad==1?196606:3,bad!=2);assert(!StaticBuildStart(17,true));assert(!normalCalls&&!linearCalls);assert(sStaticBad[17/32]&(1u<<(17%32)));tests++;}
 printf("PASS %u actual static-start/end cases: normal/partial failure/linear failure/alignment/repeated cleanup/bounds\n",tests);
}
'''
(e/'static-scratch-harness.c').write_text(head+body+main)
print('Prepared actual StaticBuildStart/End source harness; no renderer budget changes.')
