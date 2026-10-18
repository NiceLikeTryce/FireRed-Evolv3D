/* Exercise the real Emerald settings and the renderer UI adapter together. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "3ds_platform.h"
#include "fr3ds_voxel_options.h"
#include "../../3ds_port/src/3ds_settings.c"
FILE *CtrFs_OpenData(const char *name, const char *mode)
{ (void)name; (void)mode; return NULL; }
void CtrLog_Write(CtrLogCategory category, const char *format, ...)
{ (void)category; (void)format; }
void CtrDayNight_Update(void) { assert(!"Renderer controls must not change the clock"); }
int main(void)
{
    CtrSettings_Defaults();CtrSettings_Freeze();
    assert(!Fr3ds_VoxelOptionLive(FR_VOXEL_PITCH));
    Fr3ds_VoxelOptionStep(FR_VOXEL_PITCH,1);
    assert(CtrSettings_VoxelPitch()==40);
    Fr3ds_VoxelOptionStep(FR_VOXEL_ON,1);
    assert(CtrSettings_Voxel()&&Fr3ds_VoxelOptionLive(FR_VOXEL_ZOOM));
    static const int pitch[]={43,46,34,37,40};
    for(unsigned i=0;i<5;i++) {
        Fr3ds_VoxelOptionStep(FR_VOXEL_PITCH,1);
        assert(Fr3ds_VoxelOptionValue(FR_VOXEL_PITCH)==pitch[i]);
    }
    static const int zoom[]={110,120,90,100};
    for(unsigned i=0;i<4;i++) {
        Fr3ds_VoxelOptionStep(FR_VOXEL_ZOOM,1);
        assert(Fr3ds_VoxelOptionValue(FR_VOXEL_ZOOM)==zoom[i]);
    }
    Fr3ds_VoxelOptionStep(FR_VOXEL_PITCH,-1);
    Fr3ds_VoxelOptionStep(FR_VOXEL_ZOOM,-1);
    assert(CtrSettings_VoxelPitch()==37&&CtrSettings_VoxelZoom()==90);
    CtrSettings_ApplyLine("voxel_pitch=999");CtrSettings_ApplyLine("voxel_zoom=-1");
    assert(CtrSettings_VoxelPitch()==37&&CtrSettings_VoxelZoom()==90);
    assert(!Fr3ds_VoxelOptionLive(999));Fr3ds_VoxelOptionStep(999,1);
    assert(CtrSettings_GetInt("clock_speed",-1)==-1);
    assert(CtrSettings_GetInt("day_night",-1)==-1);
    puts("PASS original Emerald voxel controls: toggle, bounds, wrapping, reverse, invalid config, clock isolation");
}
