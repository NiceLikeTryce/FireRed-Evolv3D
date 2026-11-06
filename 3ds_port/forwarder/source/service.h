/* SPDX-License-Identifier: MIT */
#ifndef FRE_LAUNCHER_SERVICE_H
#define FRE_LAUNCHER_SERVICE_H
#include <stdbool.h>
#define FRE_GAME_PATH "sdmc:/3ds/FireRed-Evolv3D/FireRed-Evolv3D.3dsx"
#define FRE_APP_ROOT "sdmc:/3ds/FireRed-Evolv3D"
#define FRE_SAVE_ROOT "sdmc:/3ds/FireRedEvolved"
#define FRE_SERVICE_REQUEST FRE_SAVE_ROOT "/service.request"
bool FreService_GamePresent(void);
unsigned FreService_ConsumeRequest(void);
/* 1 requests Play;0 exits. Gfx is initialized only when entering service. */
int FreService_Run(const char *initial_error,unsigned request);
#endif
