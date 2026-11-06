/* SPDX-License-Identifier: MIT. Source-only launcher/update service. */
#include "service.h"
#include "service_manifest.h"
#include "update_http.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef FRE_SERVICE_BASE
#define FRE_SERVICE_BASE "https://niceliketryce.github.io/FireRed-Evolv3D"
#endif
#ifndef FRE_SERVICE_DIAGNOSTICS
#define FRE_SERVICE_DIAGNOSTICS 0
#endif
enum{MENU,CHECKING,AVAILABLE,APPLYING,UPDATED,ERROR_PAGE,RECOVERY_CONFIRM,SETUP_PAGE,CURRENT};
typedef struct {
    LightLock lock;Thread worker;unsigned page,job;bool busy,done;
    char phase[64],message[192];uint64_t progress,total;
    FreServiceManifest manifest;unsigned selected;
} State;
static State s;
static PrintConsole sTop,sBottom;
#if FRE_SERVICE_DIAGNOSTICS
volatile uint32_t gFreServiceQaKeys,gFreServiceQaOverride,gFreServiceQaPage,gFreServiceQaFrame;
char gFreServiceQaBase[384]=FRE_SERVICE_BASE;
#define SERVICE_BASE gFreServiceQaBase
__attribute__((naked,noinline)) void FreService_QaFrameGate(void){__asm__ volatile("nop\n\tbx lr");}
#else
#define SERVICE_BASE FRE_SERVICE_BASE
#endif
bool FreService_GamePresent(void)
{
    struct stat st;unsigned char header[44]={0},marker[8];
    if(stat(FRE_GAME_PATH,&st)||st.st_size<44||st.st_size>FRE_UPDATE_MAX_BYTES)return false;
    FILE *f=fopen(FRE_GAME_PATH,"rb");if(!f)return false;
    bool ok=fread(header,1,sizeof(header),f)==sizeof(header)&&!memcmp(header,"3DSX",4)&&(header[4]|(unsigned)header[5]<<8)>=44;
    uint32_t icon=(uint32_t)header[32]|(uint32_t)header[33]<<8|(uint32_t)header[34]<<16|(uint32_t)header[35]<<24;
    uint32_t size=(uint32_t)header[36]|(uint32_t)header[37]<<8|(uint32_t)header[38]<<16|(uint32_t)header[39]<<24;
    if(ok)ok=size>=0x2040&&icon<=(uint64_t)st.st_size&&size<=(uint64_t)st.st_size-icon;
    if(ok)ok=fseek(f,(long)(icon+0x2038),SEEK_SET)==0&&fread(marker,1,8,f)==8&&!memcmp(marker,"FREABI1",8);
    fclose(f);return ok;
}
unsigned FreService_ConsumeRequest(void)
{
    char data[32]={0};FILE *f=fopen(FRE_SERVICE_REQUEST,"rb");if(!f)return 0;
    size_t n=fread(data,1,sizeof(data)-1,f);bool complete=fgetc(f)==EOF;fclose(f);
    unsigned action=complete&&!strcmp(data,"FRESVC1:UPDATE\n")?1:complete&&!strcmp(data,"FRESVC1:RECOVER\n")?2:0;
    if(action&&n)remove(FRE_SERVICE_REQUEST);
    return action;
}
static void Progress(const char *phase,uint64_t done,uint64_t total,void *user)
{
    (void)user;LightLock_Lock(&s.lock);snprintf(s.phase,sizeof(s.phase),"%s",phase);s.progress=done;s.total=total;LightLock_Unlock(&s.lock);
}
static void ResultPage(unsigned page,const char *message)
{LightLock_Lock(&s.lock);s.page=page;snprintf(s.message,sizeof(s.message),"%s",message);LightLock_Unlock(&s.lock);}
static void Work(void *argument)
{
    unsigned job=(unsigned)(uintptr_t)argument;char error[160]={0};
    if(job==1){
        char *json=malloc(FRE_SERVICE_JSON_BYTES+1);size_t bytes=0;FreFileInfo game;
        if(!json){ResultPage(ERROR_PAGE,"Not enough memory to check updates.");goto done;}
        Progress("Connecting to updates",0,0,NULL);
        char url[512];snprintf(url,sizeof(url),"%s/service.json",SERVICE_BASE);
        bool ok=FreHttp_Init(error)&&FreHttp_Buffer(url,json,FRE_SERVICE_JSON_BYTES+1,&bytes,error);
        if(ok&&!FreService_ParseManifest(json,bytes,&s.manifest)){snprintf(error,sizeof(error),"The update information is invalid. The installed game was not changed.");ok=false;}
        free(json);
        if(!ok){ResultPage(ERROR_PAGE,error);goto done;}
        Progress("Checking installed game",0,0,NULL);
        if(!FreHashFile(FRE_GAME_PATH,&game)){ResultPage(ERROR_PAGE,"Install your generated game first. Open Setup for the wireless instructions.");goto done;}
        if(game.bytes==s.manifest.game_bytes&&FreHashEqual(game.sha256,s.manifest.game_sha)){ResultPage(CURRENT,"Your game is already current.");goto done;}
        for(unsigned i=0;i<s.manifest.patch_count;i++){
            FreServicePatch *patch=&s.manifest.patch[i];
            if(game.bytes==patch->source_bytes&&FreHashEqual(game.sha256,patch->source_sha)){
                s.selected=i;ResultPage(AVAILABLE,"An update is ready. Your saves will stay in place and the previous game will be backed up.");goto done;
            }
        }
        ResultPage(ERROR_PAGE,"This installed version needs a fresh build from the web patcher. Your game and saves were not changed.");
    }else if(job==2){
        FreServicePatch *patch=&s.manifest.patch[s.selected];char url[512];const char *download=FRE_APP_ROOT "/.update/download.bps";
        mkdir(FRE_APP_ROOT,0777);mkdir(FRE_APP_ROOT "/.update",0777);
        if(snprintf(url,sizeof(url),"%s/assets/%s",SERVICE_BASE,patch->asset)>=(int)sizeof(url)){ResultPage(ERROR_PAGE,"The update address is too long.");goto done;}
        FreFileInfo cached;bool have=FreHashFile(download,&cached)&&cached.bytes==patch->patch_bytes&&FreHashEqual(cached.sha256,patch->patch_sha);
        if(!have){
            if(remove(download)!=0&&errno!=ENOENT){ResultPage(ERROR_PAGE,"The previous download could not be replaced. Check the SD card.");goto done;}
            if(!FreHttp_Init(error)||!FreHttp_File(url,download,patch->patch_bytes,patch->patch_sha,Progress,NULL,error)){ResultPage(ERROR_PAGE,error);goto done;}
        }
        if(!FreUpdate_Install(FRE_APP_ROOT,FRE_SAVE_ROOT,download,patch->source_sha,patch->patch_sha,s.manifest.game_sha,s.manifest.game_bytes,Progress,NULL,error)){ResultPage(ERROR_PAGE,error);goto done;}
        remove(download);ResultPage(UPDATED,"Update finished. Your previous game and save snapshot are backed up. Press A to play.");
    }else if(job==3){
        Progress("Restoring previous game",0,0,NULL);
        if(FreUpdate_Rollback(FRE_APP_ROOT,FRE_SAVE_ROOT,error))ResultPage(UPDATED,"The previous game was restored. Your current saved progress was kept.");
        else ResultPage(ERROR_PAGE,error);
    }else if(job==4){
        Progress("Recovering interrupted update",0,0,NULL);
        if(FreUpdate_Recover(FRE_APP_ROOT,error))ResultPage(MENU,"Update recovery finished. Your saves remain in place.");
        else ResultPage(ERROR_PAGE,error);
    }
done:
    LightLock_Lock(&s.lock);s.done=true;LightLock_Unlock(&s.lock);
}
static bool Start(unsigned job)
{
    if(s.busy)return false;
    s.busy=true;s.done=false;s.job=job;s.progress=s.total=0;s.phase[0]=0;
    s.page=job==1?CHECKING:APPLYING;
    s.worker=threadCreate(Work,(void *)(uintptr_t)job,128*1024,0x30,0,false);
    if(!s.worker){s.busy=false;ResultPage(ERROR_PAGE,"The update worker could not start.");return false;}
    return true;
}
static unsigned TouchAction(int x,int y,unsigned page)
{
    if(x<16||x>=304)return 0;
    if(page==MENU){if(y>=16&&y<64)return KEY_A;if(y>=72&&y<116)return KEY_X;if(y>=124&&y<168)return KEY_Y;if(y>=176&&y<224)return KEY_SELECT;}
    if(y>=176&&y<232)return x<160?KEY_A:KEY_B;
    return 0;
}
static void Draw(unsigned page,bool busy,const char *phase,const char *message,uint64_t progress,uint64_t total,unsigned tick)
{
    consoleSelect(&sTop);printf("\x1b[2J\x1b[1;31mFIRERED EVOLV3D\x1b[0m\n\n");
    if(busy){printf("%s\n\n",phase[0]?phase:"Preparing");if(total>1)printf("%llu%%\n",(unsigned long long)(100*progress/total));else printf("Working %c\n","|/-\\"[(tick/8)&3]);}
    else if(page==SETUP_PAGE)printf("FIRST-TIME SETUP\n\n1. Build your game on a computer or phone:\n\n niceliketryce.github.io/FireRed-Evolv3D/\n\n2. Open the wireless companion on your PC.\n\n3. In FBI, scan its game QR code.\n\n4. Return here and choose Play.\n\nThe computer and 3DS must share home Wi-Fi.\nThe original ROM is needed only by the builder.");
    else if(page==RECOVERY_CONFIRM)printf("RESTORE PREVIOUS GAME?\n\nYour current game will be retained as a backup.\nYour current save and sidecars will stay in place.\n\nA: Restore previous game\nB: Back");
    else{printf("%s\n\n",message);if(page==AVAILABLE)printf("%s -> %s\n\nA: Save backup and update\nB: Back",s.manifest.patch[s.selected].from_version,s.manifest.version);else if(page==CURRENT||page==UPDATED)printf("A: Play\nB: Back");else if(page==ERROR_PAGE)printf("B: Back\nX: Retry update check");else printf("A: Play\nX: Check for updates\nY: Recovery\nSELECT: Setup\nSTART: Exit");}
    consoleSelect(&sBottom);printf("\x1b[2J\x1b[1;37;41m");
    if(busy)printf("\x1b[12;7H  %s  ",phase[0]?phase:"Preparing");
    else if(page==MENU)printf("\x1b[4;8H        A PLAY        \x1b[11;8H   X CHECK UPDATES    \x1b[18;8H      Y RECOVERY      \x1b[25;8H      SETUP           ");
    else printf("\x1b[25;5H     A CONFIRM        B BACK       ");
    printf("\x1b[0m");gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();
}
int FreService_Run(const char *initial_error,unsigned request)
{
    memset(&s,0,sizeof(s));LightLock_Init(&s.lock);s.page=MENU;
    if(initial_error)snprintf(s.message,sizeof(s.message),"%s",initial_error);
    else snprintf(s.message,sizeof(s.message),"Choose Play to continue your adventure.");
    gfxInitDefault();consoleInit(GFX_TOP,&sTop);consoleInit(GFX_BOTTOM,&sBottom);
    bool romfs=R_SUCCEEDED(romfsInit());
    if(!romfs)ResultPage(ERROR_PAGE,"Network certificates could not be loaded. Reinstall the launcher to use updates.");
    else if(request==1)Start(1);else if(request==3)Start(4);else if(request==2)s.page=RECOVERY_CONFIRM;
    unsigned previous=0,tick=0,pressed_touch=0;int last_x=0,last_y=0;bool was_touch=false;int play=0;
    while(aptMainLoop()){
        hidScanInput();unsigned held=hidKeysHeld();touchPosition touch;hidTouchRead(&touch);
#if FRE_SERVICE_DIAGNOSTICS
        if(gFreServiceQaOverride)held=gFreServiceQaKeys;
        gFreServiceQaPage=s.page;gFreServiceQaFrame++;FreService_QaFrameGate();
#endif
        unsigned keys=held&~previous;previous=held;bool touching=!!(held&KEY_TOUCH);
        if(touching){last_x=touch.px;last_y=touch.py;}
        LightLock_Lock(&s.lock);bool busy=s.busy,done=s.done;unsigned page=s.page;char phase[64],message[192];memcpy(phase,s.phase,sizeof(phase));memcpy(message,s.message,sizeof(message));uint64_t progress=s.progress,total=s.total;LightLock_Unlock(&s.lock);
        if(!busy&&was_touch&&!touching){unsigned action=TouchAction(last_x,last_y,page);if(action==pressed_touch)keys|=action;}
        if(touching&&!was_touch)pressed_touch=busy?0:TouchAction(last_x,last_y,page);
        was_touch=touching;
        if(done&&s.worker){threadJoin(s.worker,U64_MAX);threadFree(s.worker);s.worker=NULL;s.busy=false;s.done=false;busy=false;}
        if(!busy){
            if(keys&KEY_START)break;
            if(page==MENU){if(keys&KEY_A){play=1;break;}if(keys&KEY_X&&romfs)Start(1);else if(keys&KEY_Y)s.page=RECOVERY_CONFIRM;else if(keys&KEY_SELECT)s.page=SETUP_PAGE;}
            else if(page==AVAILABLE){if(keys&KEY_A)Start(2);else if(keys&KEY_B)s.page=MENU;}
            else if(page==RECOVERY_CONFIRM){if(keys&KEY_A)Start(3);else if(keys&KEY_B)s.page=MENU;}
            else if(page==UPDATED||page==CURRENT){if(keys&KEY_A){play=1;break;}if(keys&KEY_B)s.page=MENU;}
            else if(page==ERROR_PAGE){if(keys&KEY_B)s.page=MENU;if(keys&KEY_X&&romfs)Start(1);}
            else if(page==SETUP_PAGE&&(keys&(KEY_B|KEY_A)))s.page=MENU;
        }
        Draw(page,busy,phase,message,progress,total,++tick);
    }
    /* An OS-forced close can terminate a worker; its synced intent journal
     * is recovered next launch. Never free live networking state underneath it. */
    if(s.busy&&!s.done)exit(0);
    if(s.worker){threadJoin(s.worker,U64_MAX);threadFree(s.worker);}
    FreHttp_Shutdown();if(romfs)romfsExit();gfxExit();return play;
}
