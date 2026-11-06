/* SPDX-License-Identifier: MIT. Certificate verification stays enabled. */
#include "update_http.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <curl/curl.h>
#ifdef __3DS__
#include <3ds.h>
#include <malloc.h>
static void *sSocketBuffer;
#endif
static bool sReady;
typedef struct {
    FILE *file;char *buffer;size_t capacity;
    uint64_t bytes,limit;FreSha256 hash;FreUpdateProgress progress;void *user;
} Sink;
static size_t Receive(char *data,size_t item,size_t count,void *user)
{
    Sink *sink=user;
    if(item&&count>SIZE_MAX/item)return 0;
    size_t size=item*count;
    if(size>sink->limit-sink->bytes)return 0;
    if(sink->file){if(fwrite(data,1,size,sink->file)!=size)return 0;FreSha256_Update(&sink->hash,data,size);}
    else{if(size>sink->capacity-(size_t)sink->bytes)return 0;memcpy(sink->buffer+sink->bytes,data,size);}
    sink->bytes+=size;
    if(sink->progress)sink->progress("Downloading update",sink->bytes,sink->limit,sink->user);
    return size;
}
bool FreHttp_Init(char error[160])
{
    if(sReady)return true;
#ifdef __3DS__
    sSocketBuffer=memalign(0x1000,1024*1024);
    if(!sSocketBuffer||R_FAILED(socInit(sSocketBuffer,1024*1024))){free(sSocketBuffer);sSocketBuffer=NULL;snprintf(error,160,"Wi-Fi could not start. Check the wireless connection.");return false;}
#endif
    if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK){FreHttp_Shutdown();snprintf(error,160,"The update connection could not start.");return false;}
    sReady=true;return true;
}
void FreHttp_Shutdown(void)
{
    if(sReady)curl_global_cleanup();
    sReady=false;
#ifdef __3DS__
    if(sSocketBuffer){socExit();free(sSocketBuffer);sSocketBuffer=NULL;}
#endif
}
static bool Download(const char *url,Sink *sink,char error[160])
{
    if(!sReady){snprintf(error,160,"Update networking is not ready.");return false;}
    if(!url){snprintf(error,160,"The update address is unavailable.");return false;}
    bool https=strncmp(url,"https://",8)==0;
#if !defined(FRE_SERVICE_DIAGNOSTICS) || !FRE_SERVICE_DIAGNOSTICS
    if(!https){snprintf(error,160,"The update address must use HTTPS.");return false;}
#else
    if(!https&&strncmp(url,"http://127.",11)&&strncmp(url,"http://192.168.",15)&&strncmp(url,"http://10.",10)&&strncmp(url,"http://172.",11)){snprintf(error,160,"Invalid local diagnostic address.");return false;}
#endif
    CURL *curl=curl_easy_init();if(!curl){snprintf(error,160,"The update connection could not be created.");return false;}
    curl_easy_setopt(curl,CURLOPT_URL,url);
    curl_easy_setopt(curl,CURLOPT_PROTOCOLS_STR,https?"https":"http");
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
#ifdef __3DS__
    curl_easy_setopt(curl,CURLOPT_CAINFO,"romfs:/ca-bundle.pem");
#endif
    curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,15L);
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,sink->file?1800L:45L);
    curl_easy_setopt(curl,CURLOPT_IPRESOLVE,CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl,CURLOPT_LOW_SPEED_LIMIT,128L);
    curl_easy_setopt(curl,CURLOPT_LOW_SPEED_TIME,30L);
    curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
    curl_easy_setopt(curl,CURLOPT_USERAGENT,"FireRed-Evolv3D-Service/1");
    curl_easy_setopt(curl,CURLOPT_ACCEPT_ENCODING,"identity");
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,Receive);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,sink);
    CURLcode result=curl_easy_perform(curl);long status=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);curl_easy_cleanup(curl);
    if(result!=CURLE_OK||status!=200){snprintf(error,160,"Could not download the update (network %u, HTTP %ld). Retry or use the web patcher.",(unsigned)result,status);return false;}
    return true;
}
bool FreHttp_Buffer(const char *url,char *buffer,size_t capacity,size_t *bytes,char error[160])
{
    if(!buffer||capacity<2)return false;
    Sink sink={.buffer=buffer,.capacity=capacity-1,.limit=capacity-1};bool ok=Download(url,&sink,error);buffer[sink.bytes]=0;if(bytes)*bytes=(size_t)sink.bytes;return ok;
}
bool FreHttp_File(const char *url,const char *path,uint64_t size,const unsigned char sha[32],FreUpdateProgress progress,void *user,char error[160])
{
    if(!size||size>FRE_UPDATE_MAX_BYTES){snprintf(error,160,"Invalid update download size.");return false;}
    Sink sink={.limit=size,.progress=progress,.user=user};
    int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0666);
    sink.file=fd>=0?fdopen(fd,"wb"):NULL;
    if(fd>=0&&!sink.file)close(fd);
    if(!sink.file){snprintf(error,160,"Cannot write the update download. Check free SD card space.");return false;}
    FreSha256_Init(&sink.hash);bool ok=Download(url,&sink,error);unsigned char digest[32];FreSha256_Finish(&sink.hash,digest);
    if(fflush(sink.file))ok=false;
    if(fclose(sink.file))ok=false;
    if(ok&&(sink.bytes!=size||!FreHashEqual(digest,sha))){snprintf(error,160,"Downloaded update verification failed. The installed game is unchanged.");ok=false;}
    if(!ok)remove(path);
    return ok;
}
