/* SPDX-License-Identifier: CC-BY-NC-SA-4.0; see native_launcher_restore.c */
#ifndef FRE_NATIVE_LAUNCHER_RESTORE_H
#define FRE_NATIVE_LAUNCHER_RESTORE_H
#include <stdbool.h>
#include <stdint.h>
#define FRE_LAUNCHER_TID UINT64_C(0x000400000F3D0100)
#define FRE_LAUNCHER_RESTORE_ARG "firered-evolv3d-forwarder:hbldr-tid="
#define FRE_INSTALLED_GAME "sdmc:/3ds/FireRed-Evolv3D/FireRed-Evolv3D.3dsx"
bool FrLauncher_ParseRestore(const char *argument,uint64_t *title);
void FrLauncher_RestoreTitle(void);
#endif
