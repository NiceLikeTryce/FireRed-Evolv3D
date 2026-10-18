/* Renderer-only boundary. No Emerald game or save structures cross it. */
#ifndef FR3DS_VOXEL_OPTIONS_H
#define FR3DS_VOXEL_OPTIONS_H
#include <stdbool.h>
enum { FR_VOXEL_ON, FR_VOXEL_PITCH, FR_VOXEL_ZOOM, FR_VOXEL_ROWS };
int Fr3ds_VoxelOptionValue(unsigned row);
bool Fr3ds_VoxelOptionLive(unsigned row);
void Fr3ds_VoxelOptionStep(unsigned row, int direction);
#endif
