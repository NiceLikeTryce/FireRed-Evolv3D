/* SPDX-License-Identifier: MIT */
/* SDK-only translation unit. No game headers, enum types or saved structures. */
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "fr3ds_runtime.h"

static uint16_t sKeys;
static bool sAudio, sCsnd;
static int16_t *sCsndSamples;
static unsigned sCsndChunk, sCsndWrite, sCsndPrimed, sCsndChannel[2];
#define CSND_RING_CHUNKS 16u
#define CSND_CHANNEL_SAMPLES (CSND_RING_CHUNKS*1024u)
static ndspWaveBuf sWave[6];
static int16_t *sSamples;
static unsigned sNext;
static FILE *sLog;
bool Fr3ds_InitVideoOverride(void) __attribute__((weak));
bool Fr3ds_DrawVideoOverride(const uint16_t *pixels) __attribute__((weak));
void Fr3ds_ShutdownVideoOverride(void) __attribute__((weak));
const uint8_t *gFr3dsLastFramebuffer;

void Fr3ds_RuntimeLog(const char *message)
{
    if (sLog) { fputs(message, sLog); fputc('\n', sLog); fflush(sLog); }
}
bool Fr3ds_RuntimeInit(void)
{
    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    consoleInit(GFX_BOTTOM, NULL);
    if (R_FAILED(romfsInit())) return false;
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/FireRedEvolved", 0777);
    sLog = fopen("sdmc:/3ds/FireRedEvolved/native.log", "w");
    printf("Pokemon FireRed Evolved\nNative 3DS development build\n\nD-pad / Circle Pad: Move\nA/B: Confirm / Cancel\nSTART: Pause menu\nY: Registered item\n\nHOME: Exit\n");
    sAudio = R_SUCCEEDED(ndspInit());
    if (sAudio) {
        sSamples = linearAlloc(6 * 1024 * 2 * sizeof(int16_t));
        if (!sSamples) { ndspExit(); sAudio = false; }
    }
    if (sAudio) {
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
        ndspChnSetRate(0, 42060.0f);
        ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
        for (unsigned i = 0; i < 6; ++i) sWave[i].data_vaddr = sSamples + i * 2048;
    }
    if(!sAudio && R_SUCCEEDED(csndInit())){
        unsigned count=0;
        for(unsigned i=0;i<32&&count<2;i++)if(csndChannels&(1u<<i))sCsndChannel[count++]=i;
        sCsndSamples=linearAlloc(2*CSND_CHANNEL_SAMPLES*sizeof(int16_t));
        if(count==2&&sCsndSamples){
            memset(sCsndSamples,0,2*CSND_CHANNEL_SAMPLES*sizeof(int16_t));sCsnd=true;
        }else{if(sCsndSamples)linearFree(sCsndSamples);sCsndSamples=NULL;csndExit();}
    }
    Fr3ds_RuntimeLog("native platform initialized");
    Fr3ds_RuntimeLog(sAudio ? "audio: NDSP ready" : sCsnd ? "audio: CSND stereo fallback ready" : "audio: no available PCM transport");
    if (Fr3ds_InitVideoOverride && !Fr3ds_InitVideoOverride()) return false;
    return true;
}
uint16_t Fr3ds_RuntimeKeys(void) { return sKeys; }
bool Fr3ds_RuntimeFrame(const uint16_t *pixels)
{
    if (!aptMainLoop()) return false;
    hidScanInput();
    uint32_t keys = hidKeysHeld();
    sKeys = keys & 0x3ff;
    if (keys & KEY_Y) sKeys |= 4;
    circlePosition circle; hidCircleRead(&circle);
    if (circle.dx > 40) sKeys |= 16;
    if (circle.dx < -40) sKeys |= 32;
    if (circle.dy > 40) sKeys |= 64;
    if (circle.dy < -40) sKeys |= 128;
    if (Fr3ds_DrawVideoOverride) return Fr3ds_DrawVideoOverride(pixels);
    uint8_t *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    gFr3dsLastFramebuffer = fb;
    /* Preserve the exact game composition; 3:2 nearest scaling is 360x240. */
    for (unsigned x = 0; x < 400; ++x) for (unsigned y = 0; y < 240; ++y) {
        uint16_t c = (x < 20 || x >= 380) ? 0 : pixels[(y * 2 / 3) * 240 + (x - 20) * 2 / 3];
        unsigned at = (x * 240 + 239 - y) * 3;
        unsigned r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        fb[at] = (b << 3) | (b >> 2);
        fb[at + 1] = (g << 3) | (g >> 2);
        fb[at + 2] = (r << 3) | (r >> 2);
    }
    gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();
    return true;
}
void Fr3ds_RuntimeAudio(const int32_t *samples, uint32_t stereoFrames)
{
    if(!stereoFrames||stereoFrames>1024)return;
    if(sCsnd){
        if(sCsndChunk&&sCsndChunk!=stereoFrames)Fr3ds_RuntimeResetAudio();
        sCsndChunk=stereoFrames;
        unsigned at=sCsndWrite*sCsndChunk;
        for(unsigned i=0;i<stereoFrames;i++)for(unsigned side=0;side<2;side++){
            int32_t v=samples[i*2+side]>>8;
            sCsndSamples[side*CSND_CHANNEL_SAMPLES+at+i]=v<-32768?-32768:v>32767?32767:v;
        }
        for(unsigned side=0;side<2;side++)CSND_FlushDataCache(sCsndSamples+side*CSND_CHANNEL_SAMPLES+at,stereoFrames*2);
        sCsndWrite=(sCsndWrite+1)%CSND_RING_CHUNKS;
        if(sCsndPrimed<4&&++sCsndPrimed==4){
            bool ok=true;
            for(unsigned side=0;side<2;side++){
                int16_t *start=sCsndSamples+side*CSND_CHANNEL_SAMPLES;
                Result result=csndPlaySound(sCsndChannel[side],SOUND_FORMAT_16BIT|SOUND_REPEAT|SOUND_LINEAR_INTERP,
                    42060,1.f,side?1.f:-1.f,start,start,CSND_RING_CHUNKS*sCsndChunk*2);
                if(R_FAILED(result))ok=false;
            }
            if(ok)Fr3ds_RuntimeLog("audio: CSND stereo stream started");
            else{Fr3ds_RuntimeLog("audio: CSND start failed");csndExit();linearFree(sCsndSamples);sCsndSamples=NULL;sCsnd=false;}
        }
        return;
    }
    if(!sAudio)return;
    ndspWaveBuf *wave = &sWave[sNext];
    if (wave->status == NDSP_WBUF_QUEUED || wave->status == NDSP_WBUF_PLAYING) return;
    int16_t *dest = sSamples + sNext * 2048;
    for (unsigned i = 0; i < stereoFrames * 2; ++i) {
        int32_t v = samples[i] >> 8;
        dest[i] = v < -32768 ? -32768 : v > 32767 ? 32767 : v;
    }
    DSP_FlushDataCache(dest, stereoFrames * 4);
    wave->nsamples = stereoFrames;
    ndspChnWaveBufAdd(0, wave);
    sNext = (sNext + 1) % 6;
}
void Fr3ds_RuntimeShutdown(void)
{
    if (Fr3ds_ShutdownVideoOverride) Fr3ds_ShutdownVideoOverride();
    if (sAudio) { ndspChnWaveBufClear(0); ndspExit(); linearFree(sSamples); }
    if(sCsnd){Fr3ds_RuntimeResetAudio();csndExit();linearFree(sCsndSamples);sCsnd=false;}
    if (sLog) { fclose(sLog); sLog = NULL; }
    gfxExit();
}
void Fr3ds_RuntimeResetAudio(void)
{
    if(sCsnd){
        CSND_SetPlayState(sCsndChannel[0],0);CSND_SetPlayState(sCsndChannel[1],0);csndExecCmds(true);
        memset(sCsndSamples,0,2*CSND_CHANNEL_SAMPLES*sizeof(int16_t));
        CSND_FlushDataCache(sCsndSamples,2*CSND_CHANNEL_SAMPLES*sizeof(int16_t));
        sCsndChunk=sCsndWrite=sCsndPrimed=0;
    }
    if (!sAudio) return;
    ndspChnWaveBufClear(0);
    memset(sWave, 0, sizeof(sWave));
    for (unsigned i = 0; i < 6; ++i) sWave[i].data_vaddr = sSamples + i * 2048;
    sNext = 0;
}
