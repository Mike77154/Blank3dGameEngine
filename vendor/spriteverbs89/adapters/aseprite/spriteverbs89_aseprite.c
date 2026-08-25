#include "spriteverbs89_aseprite.h"
#include <string.h>

#define SV89_ASE_MAX_IMPORTED_FRAMES 1024

static const char *aj_find(const char *p,const char *end,const char *key)
{
    unsigned int k;
    const char *q;
    if(!p||!end||!key)return 0;
    while(p<end){if(*p=='\"'){q=p+1;k=0U;while(q<end&&key[k]&&*q==key[k]){++q;++k;}if(!key[k]&&q<end&&*q=='\"')return p;}++p;}
    return 0;
}
static const char *aj_colon(const char *p,const char *end){while(p&&p<end&&*p!=':')++p;return(p&&p<end)?p+1:0;}
static const char *aj_ws(const char*p,const char*end){while(p&&p<end&&(*p==' '||*p=='\t'||*p=='\r'||*p=='\n'))++p;return p;}
static int aj_int_after(const char *obj,const char*end,const char*key,int*out)
{
    const char *p;int sign;int v;if(!out)return 0;p=aj_find(obj,end,key);p=aj_colon(p,end);p=aj_ws(p,end);if(!p||p>=end)return 0;sign=1;if(*p=='-'){sign=-1;++p;}if(p>=end||*p<'0'||*p>'9')return 0;v=0;while(p<end&&*p>='0'&&*p<='9'){v=v*10+(*p-'0');++p;}*out=v*sign;return 1;
}
static int aj_string_after(const char*obj,const char*end,const char*key,char*out,unsigned int cap)
{
    const char*p;unsigned int n;if(!out||cap==0U)return 0;p=aj_find(obj,end,key);p=aj_colon(p,end);p=aj_ws(p,end);if(!p||p>=end||*p!='\"')return 0;++p;n=0U;while(p<end&&*p!='\"'){if(*p=='\\'&&p+1<end){++p;}if(n+1U<cap)out[n++]=*p;++p;}out[n]='\0';return p<end&&*p=='\"';
}
static const char *aj_matching_brace(const char*p,const char*end)
{
    int depth;int str;int esc;if(!p||p>=end||*p!='{')return 0;depth=0;str=0;esc=0;while(p<end){if(str){if(esc)esc=0;else if(*p=='\\')esc=1;else if(*p=='\"')str=0;}else{if(*p=='\"')str=1;else if(*p=='{')++depth;else if(*p=='}'){--depth;if(depth==0)return p;}}++p;}return 0;
}
static int aj_direction(const char*s){if(!s)return SA89_LOOP_FORWARD;if(strcmp(s,"reverse")==0)return SA89_LOOP_REVERSE;if(strcmp(s,"pingpong")==0||strcmp(s,"ping-pong")==0||strcmp(s,"pingpong_reverse")==0)return SA89_LOOP_PINGPONG;return SA89_LOOP_FORWARD;}

int sv89_aseprite_import_json(SpriteAsset89 *assets,const char*asset_name,const char*sheet_override,const char*json,unsigned int json_size,sa89_id*out_asset_id)
{
    const char *end;const char*p;const char*k;const char*brace;const char*close;char sheet[SA89_PATH_CAP];char tag[SA89_NAME_CAP];char direction[32];sa89_id source;sa89_id asset;sa89_id first;sa89_id frame_ids[SV89_ASE_MAX_IMPORTED_FRAMES];unsigned int count;int x,y,w,h,dur;int from,to;sa89_id clip;const char*meta;
    if(!assets||!asset_name||!json||json_size==0U) return 0;
    end=json+json_size;
    sheet[0]='\0';
    if(sheet_override&&sheet_override[0]) {
        unsigned int i=0U;
        while(sheet_override[i]&&i+1U<SA89_PATH_CAP){sheet[i]=sheet_override[i];++i;}
        sheet[i]='\0';
    } else {
        meta=aj_find(json,end,"meta");
        if(meta) aj_string_after(meta,end,"image",sheet,SA89_PATH_CAP);
    }
    if(!sheet[0]) return 0;
    source=sa89_add_source(assets,sheet);if(source==SA89_INVALID_ID)return 0;asset=sa89_add_asset(assets,asset_name,0,0);if(asset==SA89_INVALID_ID)return 0;
    count=0U;p=json;while(count<SV89_ASE_MAX_IMPORTED_FRAMES){k=aj_find(p,end,"frame");if(!k)break;brace=aj_colon(k,end);brace=aj_ws(brace,end);if(!brace||brace>=end){break;}if(*brace!='{'){p=brace+1;continue;}close=aj_matching_brace(brace,end);if(!close)break;if(!aj_int_after(brace,close,"x",&x)||!aj_int_after(brace,close,"y",&y)||!aj_int_after(brace,close,"w",&w)||!aj_int_after(brace,close,"h",&h)){p=close+1;continue;}dur=100;aj_int_after(close,end,"duration",&dur);first=sa89_add_frame(assets,source,0U,x,y,w,h,0,0,dur>0?(sa89_u32)dur:1U);if(first==SA89_INVALID_ID)return 0;frame_ids[count++]=first;p=close+1;}
    if(count==0U)return 0;
    p=json;clip=SA89_INVALID_ID;while((k=aj_find(p,end,"frameTags"))!=0){p=aj_colon(k,end);if(p)break;}
    if(p){while(p<end){k=aj_find(p,end,"name");if(!k)break;if(!aj_string_after(k,end,"name",tag,SA89_NAME_CAP))break;if(!aj_int_after(k,end,"from",&from)||!aj_int_after(k,end,"to",&to))break;direction[0]='\0';aj_string_after(k,end,"direction",direction,32U);if(from>=0&&to>=from&&(unsigned int)to<count){clip=sa89_add_clip(assets,asset,tag,frame_ids[from],(sa89_id)(to-from+1),aj_direction(direction));if(clip==SA89_INVALID_ID)return 0;}p=k+6;}}
    if(assets->assets[asset].clip_count==0U){clip=sa89_add_clip(assets,asset,"default",frame_ids[0],(sa89_id)count,SA89_LOOP_FORWARD);if(clip==SA89_INVALID_ID)return 0;}
    if(out_asset_id) *out_asset_id=asset;
    return 1;
}
