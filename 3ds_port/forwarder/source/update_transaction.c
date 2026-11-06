/* SPDX-License-Identifier: MIT
 * Journaled game replacement. Save files are copied to immutable snapshots,
 * never moved or overwritten by an update. Recovery uses actual file hashes,
 * not a guessed last phase, and can run again after another interruption. */
#include "update_core.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#define PATH_BYTES 512
#define JOURNAL_BYTES 124
static const char *const save_names[]={"game.sav","settings.txt","firered-evolved.nuzlocke","firered-evolved.clock"};
static void Put32(unsigned char *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(i*8));}
static uint32_t Get32(const unsigned char *p){return(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void Put64(unsigned char *p,uint64_t n){Put32(p,(uint32_t)n);Put32(p+4,(uint32_t)(n>>32));}
static uint64_t Get64(const unsigned char *p){return Get32(p)|(uint64_t)Get32(p+4)<<32;}
static bool Path(char out[PATH_BYTES],const char *root,const char *name)
{return snprintf(out,PATH_BYTES,"%s/%s",root,name)<PATH_BYTES;}
static bool Exists(const char *path){struct stat st;return stat(path,&st)==0;}
static bool Directory(const char *path)
{struct stat st;if(mkdir(path,0777)==0)return true;return errno==EEXIST&&stat(path,&st)==0&&S_ISDIR(st.st_mode);}
static bool Match(const char *path,const unsigned char sha[32],uint64_t size)
{FreFileInfo info;return FreHashFile(path,&info)&&info.bytes==size&&FreHashEqual(info.sha256,sha);}
static bool Flush(FILE *f){return fflush(f)==0&&fsync(fileno(f))==0;}
static bool SmallWrite(const char *path,const unsigned char *data,size_t bytes)
{
    FILE *f=fopen(path,"wb");if(!f)return false;
    bool ok=fwrite(data,1,bytes,f)==bytes&&Flush(f);if(fclose(f))ok=false;return ok;
}
static bool Layout(const char *root)
{
    static const char *const dirs[]={".update","backups","backups/game","backups/saves"};char path[PATH_BYTES];
    if(!Directory(root))return false;
    for(unsigned i=0;i<sizeof(dirs)/sizeof(dirs[0]);i++)if(!Path(path,root,dirs[i])||!Directory(path))return false;
    return true;
}
static bool BackupPath(char path[PATH_BYTES],const char *root,const unsigned char sha[32])
{char hex[65],name[96];FreHashToHex(sha,hex);snprintf(name,sizeof(name),"backups/game/%s.3dsx",hex);return Path(path,root,name);}
static bool CopyBackup(const char *source,const char *target,const FreFileInfo *expected)
{
    unsigned char block[16384];char temporary[PATH_BYTES];FILE *in=NULL,*out=NULL;bool ok=false;
    if(Exists(target))return Match(target,expected->sha256,expected->bytes);
    if(snprintf(temporary,sizeof(temporary),"%s.tmp",target)>=(int)sizeof(temporary))return false;
    in=fopen(source,"rb");out=fopen(temporary,"wb");
    if(in&&out){size_t n;ok=true;while((n=fread(block,1,sizeof(block),in))!=0)if(fwrite(block,1,n,out)!=n){ok=false;break;}if(ferror(in)||!Flush(out))ok=false;}
    if(in&&fclose(in))ok=false;
    if(out&&fclose(out))ok=false;
    if(ok)ok=Match(temporary,expected->sha256,expected->bytes)&&rename(temporary,target)==0;
    return ok;
}
static bool SaveSnapshot(const char *root,const char *save_root,const unsigned char game_sha[32],unsigned char snapshot[32])
{
    enum{COUNT=sizeof(save_names)/sizeof(save_names[0]),RECORD=8+32+COUNT*37+4};
    unsigned char record[RECORD];FreFileInfo info[COUNT];bool present[COUNT];char source[PATH_BYTES],folder[PATH_BYTES],target[PATH_BYTES],name[128],hex[65];
    memset(record,0,sizeof(record));memcpy(record,"FRESAVE1",8);memcpy(record+8,game_sha,32);
    for(unsigned i=0;i<COUNT;i++){
        struct stat st;if(!Path(source,save_root,save_names[i]))return false;
        present[i]=stat(source,&st)==0;
        if(!present[i]){if(errno!=ENOENT)return false;continue;}
        if(st.st_size<0||st.st_size>128*1024||!FreHashFile(source,&info[i]))return false;
        unsigned char *row=record+40+i*37;row[0]=1;Put32(row+1,(uint32_t)info[i].bytes);memcpy(row+5,info[i].sha256,32);
    }
    Put32(record+sizeof(record)-4,FreCrc32(0,record,sizeof(record)-4));
    FreSha256 hash;FreSha256_Init(&hash);FreSha256_Update(&hash,record,sizeof(record));FreSha256_Finish(&hash,snapshot);FreHashToHex(snapshot,hex);
    snprintf(name,sizeof(name),"backups/saves/%s",hex);if(!Path(folder,root,name)||!Directory(folder))return false;
    for(unsigned i=0;i<COUNT;i++)if(present[i]){
        if(!Path(source,save_root,save_names[i])||!Path(target,folder,save_names[i])||!CopyBackup(source,target,&info[i]))return false;
    }
    if(!Path(target,folder,"snapshot.bin"))return false;
    if(Exists(target)){
        unsigned char old[RECORD];FILE *f=fopen(target,"rb");if(!f)return false;
        bool ok=fread(old,1,sizeof(old),f)==sizeof(old)&&fgetc(f)==EOF&&memcmp(old,record,sizeof(old))==0;
        if(fclose(f))ok=false;
        return ok;
    }
    return SmallWrite(target,record,sizeof(record));
}
static bool ReadJournal(const char *path,unsigned char journal[JOURNAL_BYTES])
{
    FILE *f=fopen(path,"rb");if(!f)return false;
    bool ok=fread(journal,1,JOURNAL_BYTES,f)==JOURNAL_BYTES&&fgetc(f)==EOF;
    if(fclose(f))ok=false;
    return ok&&!memcmp(journal,"FRETXN1",8)&&Get32(journal+120)==FreCrc32(0,journal,120)
        &&Get64(journal+8)>0&&Get64(journal+8)<=FRE_UPDATE_MAX_BYTES
        &&Get64(journal+16)>0&&Get64(journal+16)<=FRE_UPDATE_MAX_BYTES;
}
static bool RemoveIfPresent(const char *path){return remove(path)==0||errno==ENOENT;}
static bool PreserveDifferentGame(const char *root,const char *current)
{
    FreFileInfo info;char hex[65],name[128],path[PATH_BYTES];
    if(!FreHashFile(current,&info))return false;
    FreHashToHex(info.sha256,hex);
    for(unsigned i=0;i<100;i++){
        snprintf(name,sizeof(name),"backups/game/retained-%s-%02u.3dsx",hex,i);
        if(!Path(path,root,name))return false;
        if(!Exists(path))return rename(current,path)==0;
    }
    return false;
}
bool FreUpdate_Recover(const char *root,char error[160])
{
    char current[PATH_BYTES],intent[PATH_BYTES],stage[PATH_BYTES],held[PATH_BYTES],backup[PATH_BYTES],last[PATH_BYTES];unsigned char journal[JOURNAL_BYTES];
    if(error)error[0]=0;
#define BAD(message) do{if(error)snprintf(error,160,"%s",message);return false;}while(0)
    if(!Path(current,root,"FireRed-Evolv3D.3dsx")||!Path(intent,root,".update/intent.bin")||!Path(stage,root,".update/game.new")||!Path(held,root,".update/old.held")||!Path(last,root,".update/last.bin"))BAD("Update path is too long.");
    if(!Exists(intent))return true;
    if(!ReadJournal(intent,journal)||!BackupPath(backup,root,journal+24))BAD("Update recovery data is unreadable. No game or save files were changed.");
    uint64_t old_size=Get64(journal+8),new_size=Get64(journal+16);
    bool old_current=Match(current,journal+24,old_size),new_current=Match(current,journal+56,new_size);
    bool old_held=Match(held,journal+24,old_size),old_backup=Match(backup,journal+24,old_size);
    if(new_current){
        /* The fully verified new file was activated before interruption. */
        if(!old_backup&&!old_held)BAD("The previous game backup is missing. Keep both game files and open Recovery.");
        if(old_held&&!old_backup&&rename(held,backup))BAD("Cannot retain the previous game backup.");
        if(!SmallWrite(last,journal,sizeof(journal)))BAD("Cannot finish the update recovery record.");
    }else if(!old_current){
        if(Exists(current)){
            if((!old_held&&!old_backup)||!PreserveDifferentGame(root,current))BAD("The unexpected game file was preserved. Its verified backup needs Recovery.");
        }
        if(old_held){if(rename(held,current))BAD("Cannot restore the interrupted game.");}
        else if(old_backup){if(rename(backup,current))BAD("Cannot restore the previous game.");}
        else BAD("The installed game and its update backup are unavailable. Rebuild with the patcher.");
        if(!Match(current,journal+24,old_size))BAD("Recovered game verification failed.");
    }
    if(Exists(held)){
        if(!old_held||(!old_current&&!new_current&&!Match(current,journal+24,old_size)))BAD("An unverified recovery file was preserved.");
        if(!RemoveIfPresent(held))BAD("Cannot finish cleaning the recovered update.");
    }
    if(!RemoveIfPresent(stage)||!RemoveIfPresent(intent))BAD("Cannot clear the recovered update transaction.");
    return true;
#undef BAD
}
bool FreUpdate_Install(const char *root,const char *save_root,const char *patch,
                       const unsigned char expected_source[32],const unsigned char expected_patch[32],
                       const unsigned char expected_target[32],uint64_t target_bytes,
                       FreUpdateProgress progress,void *user,char error[160])
{
    char current[PATH_BYTES],intent[PATH_BYTES],intent_tmp[PATH_BYTES],stage[PATH_BYTES],held[PATH_BYTES],backup[PATH_BYTES];
    FreFileInfo source;unsigned char journal[JOURNAL_BYTES];bool ok=false;bool intent_created=false;
    if(error)error[0]=0;
#define FAIL(message) do{if(error)snprintf(error,160,"%s",message);goto done;}while(0)
    if(!Layout(root)||!Path(current,root,"FireRed-Evolv3D.3dsx")||!Path(intent,root,".update/intent.bin")||!Path(intent_tmp,root,".update/intent.tmp")||!Path(stage,root,".update/game.new")||!Path(held,root,".update/old.held")||!BackupPath(backup,root,expected_source))FAIL("Cannot create the update folders. Check the SD card.");
    if(!FreUpdate_Recover(root,error))goto done;
    if(Exists(stage)||Exists(held))FAIL("An earlier staged file was preserved. Open Recovery before updating.");
    if(!FreHashFile(current,&source)||!FreHashEqual(source.sha256,expected_source))FAIL("This update does not match the installed game. Use the web patcher for a fresh build.");
    if(FreHashEqual(expected_source,expected_target)||!target_bytes||target_bytes>FRE_UPDATE_MAX_BYTES)FAIL("This update is not a new valid game version.");
    memset(journal,0,sizeof(journal));memcpy(journal,"FRETXN1",8);Put64(journal+8,source.bytes);Put64(journal+16,target_bytes);memcpy(journal+24,expected_source,32);memcpy(journal+56,expected_target,32);
    if(progress)progress("Backing up saves",0,1,user);
    if(!SaveSnapshot(root,save_root,expected_source,journal+88))FAIL("Save backup could not finish. The installed game and saves are unchanged.");
    Put32(journal+120,FreCrc32(0,journal,120));
    if(!SmallWrite(intent_tmp,journal,sizeof(journal))||rename(intent_tmp,intent))FAIL("Cannot record the update transaction.");
    intent_created=true;
    if(!FreBps_Apply(current,patch,stage,expected_source,expected_patch,expected_target,target_bytes,progress,user,error))goto done;
    /* Flush staged data before any rename; the original still exists here. */
    FILE *file=fopen(stage,"r+b");if(!file)FAIL("Cannot verify the staged game file.");
    bool synced=Flush(file);if(fclose(file))synced=false;if(!synced)FAIL("Cannot flush the staged game to the SD card.");
    if(progress)progress("Installing verified game",0,1,user);
    if(Exists(backup)){
        if(!Match(backup,expected_source,source.bytes)||rename(current,held))FAIL("Cannot preserve the original game backup.");
    }else if(rename(current,backup))FAIL("Cannot move the original game into its backup.");
    if(rename(stage,current))FAIL("Cannot activate the updated game. Recovery will restore the previous version.");
    if(!Match(current,expected_target,target_bytes))FAIL("The activated game did not verify. Its backup has been preserved.");
    if(!FreUpdate_Recover(root,error))goto done;
    ok=true;
done:
    if(!ok&&intent_created){
        char recovery[160];
        if(!FreUpdate_Recover(root,recovery)&&error){size_t used=strlen(error);if(used<100)snprintf(error+used,160-used," Recovery: %.50s",recovery);}
    }
    return ok;
#undef FAIL
}

bool FreUpdate_Rollback(const char *root,const char *save_root,char error[160])
{
    char current[PATH_BYTES],last[PATH_BYTES],intent[PATH_BYTES],temporary[PATH_BYTES],old_backup[PATH_BYTES],new_backup[PATH_BYTES],held[PATH_BYTES];
    unsigned char previous[JOURNAL_BYTES],journal[JOURNAL_BYTES];FreFileInfo current_info;bool recorded=false,ok=false;
    if(error)error[0]=0;
#define FAIL(message) do{if(error)snprintf(error,160,"%s",message);goto done;}while(0)
    if(!Layout(root)||!FreUpdate_Recover(root,error))goto done;
    if(!Path(current,root,"FireRed-Evolv3D.3dsx")||!Path(last,root,".update/last.bin")||!Path(intent,root,".update/intent.bin")||!Path(temporary,root,".update/intent.tmp")||!Path(held,root,".update/old.held"))FAIL("Recovery path is too long.");
    if(!ReadJournal(last,previous)||!BackupPath(old_backup,root,previous+24))FAIL("No previous verified game is recorded.");
    if(Match(current,previous+24,Get64(previous+8)))return true;
    if(!Match(old_backup,previous+24,Get64(previous+8)))FAIL("The previous game backup is unavailable. No saves were changed.");
    if(!Match(current,previous+56,Get64(previous+16))){
        unsigned char snapshot[32];
        if(!SaveSnapshot(root,save_root,previous+56,snapshot))FAIL("Current save backup did not finish.");
        if(!SmallWrite(temporary,previous,sizeof(previous))||rename(temporary,intent))FAIL("Cannot record previous-game recovery.");
        recorded=true;ok=FreUpdate_Recover(root,error);goto done;
    }
    if(!FreHashFile(current,&current_info))FAIL("The current game cannot be read. No saves were changed.");
    if(!BackupPath(new_backup,root,current_info.sha256))FAIL("Recovery backup path is too long.");
    memset(journal,0,sizeof(journal));memcpy(journal,"FRETXN1",8);Put64(journal+8,current_info.bytes);Put64(journal+16,Get64(previous+8));memcpy(journal+24,current_info.sha256,32);memcpy(journal+56,previous+24,32);
    if(!SaveSnapshot(root,save_root,current_info.sha256,journal+88))FAIL("Current save backup did not finish. Recovery was not started.");
    Put32(journal+120,FreCrc32(0,journal,120));
    if(!SmallWrite(temporary,journal,sizeof(journal))||rename(temporary,intent))FAIL("Cannot record the recovery transaction.");
    recorded=true;
    if(Exists(new_backup)){
        if(!Match(new_backup,current_info.sha256,current_info.bytes)||Exists(held)||rename(current,held))FAIL("Cannot preserve the current game version.");
    }else if(rename(current,new_backup))FAIL("Cannot back up the current game version.");
    if(rename(old_backup,current))FAIL("The previous game could not be restored. Automatic recovery will preserve a complete version.");
    if(!FreUpdate_Recover(root,error))goto done;
    ok=true;
done:
    if(!ok&&recorded){char ignored[160];FreUpdate_Recover(root,ignored);}
    return ok;
#undef FAIL
}
