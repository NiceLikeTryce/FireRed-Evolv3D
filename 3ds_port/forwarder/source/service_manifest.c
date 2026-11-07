/* SPDX-License-Identifier: MIT */
#include "service_manifest.h"
#define JSMN_STRICT
#define JSMN_PARENT_LINKS
#define JSMN_STATIC
#include "vendor/jsmn.h"
#include <string.h>
typedef struct{const char *text;jsmntok_t token[256];int count;} Json;
static int Skip(const Json *j,int at)
{
    int end=j->token[at].end;at++;
    while(at<j->count&&j->token[at].start<end)at++;
    return at;
}
static bool Equal(const Json *j,int at,const char *word)
{return at>=0&&j->token[at].type==JSMN_STRING&&j->token[at].end-j->token[at].start==(int)strlen(word)&&!memcmp(j->text+j->token[at].start,word,strlen(word));}
static int Property(const Json *j,int object,const char *key)
{
    if(object<0||j->token[object].type!=JSMN_OBJECT)return-1;
    int found=-1;
    for(int at=object+1;at<j->count&&j->token[at].start<j->token[object].end;){
        if(j->token[at].type!=JSMN_STRING||at+1>=j->count)return-1;
        if(Equal(j,at,key)){if(found>=0)return-1;found=at+1;}
        at=Skip(j,at+1);
    }
    return found;
}
static bool Text(const Json *j,int at,char *out,size_t size)
{
    if(at<0||j->token[at].type!=JSMN_STRING)return false;
    int bytes=j->token[at].end-j->token[at].start;
    if(bytes<=0||(size_t)bytes>=size)return false;
    const char *p=j->text+j->token[at].start;
    for(int i=0;i<bytes;i++)if((unsigned char)p[i]<32||(unsigned char)p[i]>126||p[i]=='\\')return false;
    memcpy(out,p,(size_t)bytes);out[bytes]=0;return true;
}
static bool Number(const Json *j,int at,uint64_t *out)
{
    if(at<0||j->token[at].type!=JSMN_PRIMITIVE)return false;
    int start=j->token[at].start,end=j->token[at].end;uint64_t value=0;
    if(start>=end||(end-start>1&&j->text[start]=='0'))return false;
    for(int i=start;i<end;i++){
        unsigned digit=(unsigned char)j->text[i]-'0';
        if(digit>9||value>(UINT64_MAX-digit)/10u)return false;
        value=value*10+digit;
    }
    *out=value;return true;
}
static bool Version(const char *s)
{
    if(*s++!='v')return false;
    for(unsigned part=0;part<3;part++){
        const char *begin=s;
        while(*s>='0'&&*s<='9')s++;
        if(s==begin||(s-begin>1&&*begin=='0'))return false;
        if(part<2){if(*s++!='.')return false;}else if(*s)return false;
    }
    return true;
}
static bool Hash(const Json *j,int at,unsigned char out[32])
{char text[65];return Text(j,at,text,sizeof(text))&&FreHashFromHex(text,out);}
static int CompareVersion(const char *a,const char *b)
{
    a++;b++;
    for(unsigned i=0;i<3;i++){
        const char *ae=a,*be=b;while(*ae&&*ae!='.')ae++;while(*be&&*be!='.')be++;
        size_t an=(size_t)(ae-a),bn=(size_t)(be-b);if(an!=bn)return an>bn?1:-1;
        int result=memcmp(a,b,an);if(result)return result>0?1:-1;
        a=*ae?ae+1:ae;b=*be?be+1:be;
    }
    return 0;
}
static bool Asset(const char *s)
{
    size_t n=strlen(s);if(n<5||strcmp(s+n-4,".bps")||strstr(s,".."))return false;
    for(size_t i=0;i<n;i++)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]=='-'||s[i]=='_'||s[i]=='.'))return false;
    return true;
}
static bool Primitive(const char *p,int size)
{
    if((size==4&&!memcmp(p,"true",4))||(size==5&&!memcmp(p,"false",5))||(size==4&&!memcmp(p,"null",4)))return true;
    int i=0;if(i<size&&p[i]=='-')i++;
    if(i>=size)return false;
    if(p[i]=='0')i++;
    else{if(p[i]<'1'||p[i]>'9')return false;while(i<size&&p[i]>='0'&&p[i]<='9')i++;}
    if(i<size&&p[i]=='.'){int start=++i;while(i<size&&p[i]>='0'&&p[i]<='9')i++;if(i==start)return false;}
    if(i<size&&(p[i]=='e'||p[i]=='E')){i++;if(i<size&&(p[i]=='+'||p[i]=='-'))i++;int start=i;while(i<size&&p[i]>='0'&&p[i]<='9')i++;if(i==start)return false;}
    return i==size;
}
bool FreService_ParseManifest(const char *text,size_t bytes,FreServiceManifest *out)
{
    if(!text||!out||bytes==0||bytes>FRE_SERVICE_JSON_BYTES)return false;
    /* Producer uses ASCII JSON (Unicode, if ever present, is escaped). */
    for(size_t i=0;i<bytes;i++)if((unsigned char)text[i]>126||((unsigned char)text[i]<32&&text[i]!='\n'&&text[i]!='\r'&&text[i]!='\t'))return false;
    Json j={.text=text};jsmn_parser parser;jsmn_init(&parser);j.count=jsmn_parse(&parser,text,bytes,j.token,256);
    if(j.count<1||j.token[0].type!=JSMN_OBJECT||Skip(&j,0)!=j.count)return false;
    for(int i=0;i<j.count;i++)if(j.token[i].type==JSMN_PRIMITIVE&&!Primitive(text+j.token[i].start,j.token[i].end-j.token[i].start))return false;
    uint64_t schema;int output,patches;
    if(!Number(&j,Property(&j,0,"schema"),&schema)||schema!=1||!Equal(&j,Property(&j,0,"application"),"FireRed-Evolv3D"))return false;
    memset(out,0,sizeof(*out));
    if(!Text(&j,Property(&j,0,"version"),out->version,sizeof(out->version))||!Version(out->version))return false;
    output=Property(&j,0,"output");
    if(!Number(&j,Property(&j,output,"size"),&out->game_bytes)||!out->game_bytes||out->game_bytes>FRE_UPDATE_MAX_BYTES||!Hash(&j,Property(&j,output,"sha256"),out->game_sha))return false;
    patches=Property(&j,0,"patches");
    if(patches<0||j.token[patches].type!=JSMN_ARRAY||j.token[patches].size>FRE_SERVICE_PATCHES)return false;
    for(int at=patches+1;at<j.count&&j.token[at].start<j.token[patches].end;at=Skip(&j,at)){
        if(out->patch_count>=FRE_SERVICE_PATCHES||j.token[at].type!=JSMN_OBJECT)return false;
        FreServicePatch *p=&out->patch[out->patch_count];
        if(!Text(&j,Property(&j,at,"from_version"),p->from_version,sizeof(p->from_version))||!Version(p->from_version)||CompareVersion(out->version,p->from_version)<=0
           ||!Text(&j,Property(&j,at,"asset"),p->asset,sizeof(p->asset))||!Asset(p->asset)
           ||!Number(&j,Property(&j,at,"source_size"),&p->source_bytes)||!p->source_bytes||p->source_bytes>FRE_UPDATE_MAX_BYTES
           ||!Number(&j,Property(&j,at,"size"),&p->patch_bytes)||p->patch_bytes<16||p->patch_bytes>FRE_UPDATE_MAX_BYTES
           ||!Hash(&j,Property(&j,at,"source_sha256"),p->source_sha)||!Hash(&j,Property(&j,at,"sha256"),p->patch_sha)
           ||FreHashEqual(p->source_sha,out->game_sha))return false;
        for(unsigned i=0;i<out->patch_count;i++)if(FreHashEqual(out->patch[i].source_sha,p->source_sha))return false;
        out->patch_count++;
    }
    if (out->patch_count != (unsigned)j.token[patches].size)
        return false;
    /* Optional display text. Older manifests and malformed notes keep the
     * normal update confirmation. Text rejects controls and JSON escapes. */
    int notes = Property(&j, 0, "notes");
    if (notes >= 0 && j.token[notes].type == JSMN_ARRAY
        && j.token[notes].size <= FRE_SERVICE_NOTE_LINES) {
        for (int at = notes + 1; at < j.count && j.token[at].start < j.token[notes].end; at = Skip(&j, at)) {
            if (out->note_count >= FRE_SERVICE_NOTE_LINES
                || !Text(&j, at, out->notes[out->note_count], sizeof(out->notes[0]))) {
                out->note_count = 0;
                break;
            }
            ++out->note_count;
        }
    }
    return true;
}
