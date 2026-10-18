/* Adapt the renderer controls from 3ds_bottom_ui.c's ActivateOption.
 * Keep Emerald3DS's settings implementation and bounded camera steps.
 * Unsupported blur and battle compositors are deliberately not advertised.
 * Original upstream notices/licences remain in this source fork.
 */
#include "3ds_platform.h"
#include "fr3ds_voxel_options.h"
int Fr3ds_VoxelOptionValue(unsigned row)
{
    switch(row) {
    case FR_VOXEL_ON: return CtrSettings_Voxel();
    case FR_VOXEL_PITCH: return CtrSettings_VoxelPitch();
    case FR_VOXEL_ZOOM: return CtrSettings_VoxelZoom();
    default: return 0;
    }
}
bool Fr3ds_VoxelOptionLive(unsigned row)
{
    return row < FR_VOXEL_ROWS && (row == FR_VOXEL_ON || CtrSettings_Voxel());
}
void Fr3ds_VoxelOptionStep(unsigned row, int direction)
{
    if (!Fr3ds_VoxelOptionLive(row)) return;
    switch(row) {
    case FR_VOXEL_ON: CtrSettings_SetVoxel(!CtrSettings_Voxel()); break;
    case FR_VOXEL_PITCH: CtrSettings_StepVoxelPitch(direction); break;
    case FR_VOXEL_ZOOM: CtrSettings_StepVoxelZoom(direction); break;
    }
}
