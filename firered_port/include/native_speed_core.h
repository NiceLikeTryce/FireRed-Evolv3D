/* SPDX-License-Identifier: MIT
 * Bounded frame selection; model follows Emerald3DS FrameShown/PaceBehind.
 * Game callbacks always run. Only presentation and fast-forward sound ticks
 * may be withheld; sleep/load stalls never create a catch-up burst. */
#ifndef FR_NATIVE_SPEED_CORE_H
#define FR_NATIVE_SPEED_CORE_H
#include <stdint.h>
#include <stdbool.h>
typedef struct { uint64_t start, shown; unsigned frames,earned,hidden,mode; bool ready; } FrSpeedState;
static inline bool FrSpeed_Frame(FrSpeedState *s,uint64_t us,unsigned halves,bool pace,bool link,bool *sound)
{
    if(halves<2||halves>8)halves=2;
    unsigned mode=(link?2:halves)|(pace&&!link&&halves<=2?256:0);
    if(link)halves=2;
    if(!s->ready||s->mode!=mode){*s=(FrSpeedState){.start=us,.shown=us,.mode=mode,.ready=true};}
    bool draw=true;
    if(mode&256){
        ++s->frames;
        uint64_t elapsed=us>=s->start?us-s->start:0;
        uint64_t expected=(uint64_t)s->frames*16715u;
        if(elapsed>expected+66860u||elapsed+16715u<expected){s->start=us;s->frames=0;s->hidden=0;}
        else if(elapsed>=expected+16715u&&!s->hidden){draw=false;s->hidden=1;}
        else s->hidden=0;
        *sound=true;
    }else{
        s->earned+=2;
        if(halves>2&&s->earned<halves&&us>=s->shown&&us-s->shown<16715u)draw=false;
        else{s->earned=s->earned>=halves?s->earned-halves:0;}
        *sound=draw;
    }
    if(draw)s->shown=us;
    return draw;
}
#endif
