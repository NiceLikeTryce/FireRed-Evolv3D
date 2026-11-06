/* SPDX-License-Identifier: CC-BY-NC-SA-4.0
 * Luma hb:ldr title restoration. Protocol adapted from Emerald3DS
 * bcae4b70dac61419fc933084084469f6cfbe344b, CC BY-NC-SA4.0.
 * Emerald 3DS by Zallax and contributors: github.com/ZallaxDev/emerald-3ds.
 * The native FireRed backend has a separate main, so it needs this hook. */
#include "native_launcher_restore.h"
#include <stddef.h>
#include <string.h>
bool FrLauncher_ParseRestore(const char *argument,uint64_t *title)
{
    static const char prefix[]=FRE_LAUNCHER_RESTORE_ARG;
    uint64_t value=0;
    if(!argument||!title||strncmp(argument,prefix,sizeof(prefix)-1))return false;
    argument+=sizeof(prefix)-1;
    for(unsigned i=0;i<16;i++){
        unsigned char c=(unsigned char)argument[i];unsigned digit;
        if(c>='0'&&c<='9')digit=c-'0';
        else if(c>='a'&&c<='f')digit=c-'a'+10;
        else if(c>='A'&&c<='F')digit=c-'A'+10;
        else return false;
        value=(value<<4)|digit;
    }
    if(argument[16])return false;
    /* Zero restores Luma's original default-selection state too. */
    *title=value;return true;
}
#ifdef __3DS__
#include <3ds.h>
extern int __system_argc;
extern char **__system_argv;
volatile uint32_t gFrLauncherRestoreStatus;
void FrLauncher_RestoreTitle(void)
{
    if(gFrLauncherRestoreStatus)return;
    for(int i=1;i<__system_argc;i++){
        uint64_t title;s64 luma;
        if(!FrLauncher_ParseRestore(__system_argv[i],&title))continue;
        if(R_SUCCEEDED(svcGetSystemInfo(&luma,0x10000,0))){
            *(volatile u64 *)(OS_SHAREDCFG_VADDR+0x808)=title;
            gFrLauncherRestoreStatus=1;
        }else gFrLauncherRestoreStatus=2;
        return;
    }
}
__attribute__((constructor)) static void RestoreLauncherTitle(void){FrLauncher_RestoreTitle();}
#endif
