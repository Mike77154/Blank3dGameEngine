#include "spriteasset89_renlist89.h"

static int rl_space(int c){return c==' '||c=='\t'||c=='\r';}
static void rl_copy(char*d,unsigned int cap,const char*s,unsigned int n){unsigned int i=0U;if(!d||cap==0U)return;while(i<n&&i+1U<cap){d[i]=s[i];++i;}d[i]='\0';}
static int rl_starts(const char*s,unsigned int n,const char*p){unsigned int i=0U;while(p[i]){if(i>=n||s[i]!=p[i])return 0;++i;}return 1;}
static unsigned int rl_trim_left(const char*s,unsigned int n){unsigned int i=0U;while(i<n&&rl_space((unsigned char)s[i]))++i;return i;}
static unsigned int rl_trim_right(const char*s,unsigned int n){while(n>0U&&rl_space((unsigned char)s[n-1U]))--n;return n;}
static int rl_ms(const char*s,unsigned int n,unsigned int*out){unsigned int i=0U,whole=0U,frac=0U,scale=1U;if(!s||!out)return 0;while(i<n&&rl_space((unsigned char)s[i]))++i;if(i>=n||s[i]<'0'||s[i]>'9')return 0;while(i<n&&s[i]>='0'&&s[i]<='9'){whole=whole*10U+(unsigned int)(s[i]-'0');++i;}if(i<n&&s[i]=='.'){++i;while(i<n&&s[i]>='0'&&s[i]<='9'&&scale<10000U){frac=frac*10U+(unsigned int)(s[i]-'0');scale*=10U;++i;}}*out=whole*1000U+(frac*1000U)/scale;return 1;}
int sa89_renlist_import(SpriteAsset89*ctx,const char*text,unsigned int size,sa89_id*out_asset_id)
{
    unsigned int pos=0U,start,n,l,r,ms;char asset_name[SA89_NAME_CAP];char path[SA89_PATH_CAP];sa89_id asset=SA89_INVALID_ID,source,first=SA89_INVALID_ID,last_frame=SA89_INVALID_ID;unsigned short count=0U;int loop=SA89_LOOP_NONE;
    if(!ctx||!text||size==0U)return 0;
    while(pos<size){start=pos;while(pos<size&&text[pos]!='\n')++pos;n=pos-start;if(pos<size)++pos;l=rl_trim_left(text+start,n);r=rl_trim_right(text+start+l,n-l);if(r==0U||text[start+l]=='#')continue;
        if(rl_starts(text+start+l,r,"image ")){unsigned int off=6U;unsigned int len=r-off;if(len&&text[start+l+off+len-1U]==':')--len;rl_copy(asset_name,SA89_NAME_CAP,text+start+l+off,len);asset=sa89_add_asset(ctx,asset_name,0,0);if(asset==SA89_INVALID_ID)return 0;first=ctx->frame_count;}
        else if(text[start+l]=='\"'&&asset!=SA89_INVALID_ID){unsigned int e=1U;while(e<r&&text[start+l+e]!='\"')++e;if(e>=r)return 0;rl_copy(path,SA89_PATH_CAP,text+start+l+1U,e-1U);source=sa89_add_source(ctx,path);if(source==SA89_INVALID_ID)return 0;last_frame=sa89_add_frame(ctx,source,0U,0,0,0,0,0,0,100U);if(last_frame==SA89_INVALID_ID)return 0;++count;}
        else if(rl_starts(text+start+l,r,"pause ")&&last_frame!=SA89_INVALID_ID){if(!rl_ms(text+start+l+6U,r-6U,&ms))return 0;ctx->frames[last_frame].duration_ms=ms?ms:1U;}
        else if(rl_starts(text+start+l,r,"pause_ms ")&&last_frame!=SA89_INVALID_ID){unsigned int i=9U;ms=0U;while(i<r&&text[start+l+i]>='0'&&text[start+l+i]<='9'){ms=ms*10U+(unsigned int)(text[start+l+i]-'0');++i;}ctx->frames[last_frame].duration_ms=ms?ms:1U;}
        else if(rl_starts(text+start+l,r,"repeat"))loop=SA89_LOOP_FORWARD;
        else if(rl_starts(text+start+l,r,"pingpong"))loop=SA89_LOOP_PINGPONG;
    }
    if(asset==SA89_INVALID_ID||first==SA89_INVALID_ID||count==0U) return 0;
    if(sa89_add_clip(ctx,asset,"default",first,count,loop)==SA89_INVALID_ID) return 0;
    if(out_asset_id) *out_asset_id=asset;
    return 1;
}
