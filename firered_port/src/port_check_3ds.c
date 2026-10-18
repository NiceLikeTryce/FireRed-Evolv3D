/* SPDX-License-Identifier: MIT */
#include <3ds.h>
#include <stdio.h>
#include "fr3ds_bridge.h"
#ifndef FR3DS_PRIVATE_CORE
void Fr3ds_CoreInit(void){}
void Fr3ds_CoreTick(uint16_t down){(void)down;}
Fr3dsCoreInfo Fr3ds_CoreInfo(void){Fr3dsCoreInfo i={"platform bridge only",false,false,251,0,0};return i;}
#endif
int main(void) {
    gfxInitDefault();consoleInit(GFX_TOP,NULL);
    printf("FireRed Evolved3DS\nNative port check\n\nNOT A PLAYABLE GAME BUILD\n");
    Fr3ds_ResetHardware();Fr3ds_CoreInit();
    while(aptMainLoop()) {
        hidScanInput();circlePosition stick;hidCircleRead(&stick);touchPosition touch;hidTouchRead(&touch);
        CtrInputSample sample={(uint16_t)(hidKeysHeld()&0xfff),stick.dx,stick.dy,(hidKeysHeld()&KEY_TOUCH)!=0,touch.px,touch.py};
        uint16_t held=Fr3ds_UpdateInput(&sample);Fr3ds_CoreTick((uint16_t)hidKeysDown());Fr3dsCoreInfo i=Fr3ds_CoreInfo();
        printf("\x1b[6;0HCore: %-32s\nNative core: %s\nFull game:   %s\nDex: %u Tasks: %u\nRNG: %04x Input: %03x\n\nA: exercise native RNG\nSTART: exit check\n",i.name,i.nativeCore?"linked":"not linked",i.playable?"linked":"NOT LINKED",i.dexLimit,i.taskCount,i.randomValue,held);
        if(hidKeysDown()&KEY_START)break;
        gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();
    }
    gfxExit();return 0;
}
