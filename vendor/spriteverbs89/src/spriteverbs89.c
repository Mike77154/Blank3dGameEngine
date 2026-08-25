#include "spriteverbs89.h"
#include <string.h>
#include <stdlib.h>

#define SV89_ERR_NONE 0
#define SV89_ERR_ARGUMENT 1
#define SV89_ERR_CAPACITY 2
#define SV89_ERR_TARGET 3
#define SV89_ERR_ASSET 4
#define SV89_ERR_COMMAND 5

static void sv89_zero(void *p, unsigned int n)
{
    unsigned char *b;
    unsigned int i;
    b = (unsigned char *)p;
    for (i = 0U; i < n; ++i) b[i] = 0U;
}

static void sv89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    i = 0U;
    if (src) while (src[i] && i + 1U < cap) { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}

static int sv89_eq(const char *a, const char *b)
{
    return a && b && strcmp(a,b)==0;
}

void sv89_init(SpriteVerbs89 *ctx, SpriteAsset89 *assets)
{
    if (!ctx) return;
    sv89_zero(ctx, (unsigned int)sizeof(*ctx));
    ctx->assets = assets;
}

void sv89_set_lazy_asset_provider(SpriteVerbs89 *ctx, sv89_lazy_asset_fn fn, void *user)
{
    if (!ctx) return;
    ctx->lazy_asset = fn;
    ctx->lazy_asset_user = user;
}

int sv89_resolve_verb(const char *v)
{
    if (!v) return SV89_VERB_UNKNOWN;
    if (sv89_eq(v,"show") || sv89_eq(v,"image_show") || sv89_eq(v,"sprite_show")) return SV89_VERB_SHOW;
    if (sv89_eq(v,"hide") || sv89_eq(v,"image_hide") || sv89_eq(v,"sprite_hide")) return SV89_VERB_HIDE;
    if (sv89_eq(v,"play") || sv89_eq(v,"animate") || sv89_eq(v,"sprite_play") || sv89_eq(v,"play_sprite")) return SV89_VERB_PLAY;
    if (sv89_eq(v,"stop") || sv89_eq(v,"sprite_stop")) return SV89_VERB_STOP;
    if (sv89_eq(v,"sprite") || sv89_eq(v,"image") || sv89_eq(v,"sprite_set") || sv89_eq(v,"sprite_index")) return SV89_VERB_SET_ASSET;
    if (sv89_eq(v,"frame") || sv89_eq(v,"sprite_frame") || sv89_eq(v,"image_index")) return SV89_VERB_SET_FRAME;
    if (sv89_eq(v,"speed") || sv89_eq(v,"sprite_speed") || sv89_eq(v,"image_speed")) return SV89_VERB_SET_SPEED;
    if (sv89_eq(v,"reset") || sv89_eq(v,"sprite_reset")) return SV89_VERB_RESET;
    if (sv89_eq(v,"flip_x") || sv89_eq(v,"sprite_flip_x")) return SV89_VERB_FLIP_X;
    if (sv89_eq(v,"flip_y") || sv89_eq(v,"sprite_flip_y")) return SV89_VERB_FLIP_Y;
    if (sv89_eq(v,"x") || sv89_eq(v,"sprite_x")) return SV89_VERB_SET_X;
    if (sv89_eq(v,"y") || sv89_eq(v,"sprite_y")) return SV89_VERB_SET_Y;
    if (sv89_eq(v,"pos") || sv89_eq(v,"position") || sv89_eq(v,"sprite_pos") || sv89_eq(v,"sprite_position")) return SV89_VERB_SET_POS;
    return SV89_VERB_UNKNOWN;
}

sa89_id sv89_target_player(const SpriteVerbs89 *ctx, const char *name)
{
    unsigned short i;
    if (!ctx || !name) return SA89_INVALID_ID;
    for (i=0U;i<ctx->target_count;++i) if(ctx->targets[i].used && strcmp(ctx->targets[i].name,name)==0) return ctx->targets[i].player_id;
    return SA89_INVALID_ID;
}

sa89_id sv89_bind_target(SpriteVerbs89 *ctx, const char *name)
{
    sa89_id p;
    SV89_Target *t;
    if (!ctx || !ctx->assets || !name || !name[0]) return SA89_INVALID_ID;
    p = sv89_target_player(ctx,name);
    if (p != SA89_INVALID_ID) return p;
    if (ctx->target_count >= SV89_MAX_TARGETS) { ctx->last_error=SV89_ERR_CAPACITY; return SA89_INVALID_ID; }
    p = sa89_player_create(ctx->assets);
    if (p == SA89_INVALID_ID) return p;
    t=&ctx->targets[ctx->target_count++];
    sv89_zero(t,(unsigned int)sizeof(*t));
    sv89_copy(t->name,SV89_TARGET_CAP,name);
    t->player_id=p; t->used=1U;
    return p;
}

static void sv89_split_asset_clip(const char *arg, char *asset, unsigned int acap, char *clip, unsigned int ccap)
{
    unsigned int i,j;
    int sep;
    if (asset && acap) asset[0]='\0';
    if (clip && ccap) clip[0]='\0';
    if (!arg) return;
    i=0U; sep=-1;
    while(arg[i]){ if(arg[i]==':' || arg[i]=='#') {sep=(int)i;break;} ++i; }
    if(sep<0){ sv89_copy(asset,acap,arg); return; }
    i=0U; while(arg[i] && (int)i<sep && i+1U<acap){asset[i]=arg[i];++i;} asset[i]='\0';
    j=0U;i=(unsigned int)sep+1U;while(arg[i]&&j+1U<ccap)clip[j++]=arg[i++];clip[j]='\0';
}

static sa89_id sv89_resolve_asset(SpriteVerbs89 *ctx, const char *name)
{
    sa89_id a;
    if (!ctx || !ctx->assets || !name || !name[0]) return SA89_INVALID_ID;
    a=sa89_find_asset(ctx->assets,name);
    if(a==SA89_INVALID_ID && ctx->lazy_asset){
        if(!ctx->lazy_asset(ctx->lazy_asset_user,ctx->assets,name,&a)) a=SA89_INVALID_ID;
    }
    return a;
}

int sv89_parse_q16(const char *text, int *out_q16)
{
    int sign;
    unsigned int whole;
    unsigned int frac;
    unsigned int scale;
    unsigned int i;
    if(!text||!out_q16)return 0;
    i=0U;sign=1;if(text[i]=='-'){sign=-1;++i;}else if(text[i]=='+')++i;
    if(text[i]<'0'||text[i]>'9')return 0;
    whole=0U;while(text[i]>='0'&&text[i]<='9'){if(whole>32767U)return 0;whole=whole*10U+(unsigned int)(text[i]-'0');++i;}
    frac=0U;scale=1U;if(text[i]=='.'){++i;while(text[i]>='0'&&text[i]<='9'&&scale<100000U){frac=frac*10U+(unsigned int)(text[i]-'0');scale*=10U;++i;}while(text[i]>='0'&&text[i]<='9')++i;}
    if(text[i]!='\0')return 0;
    *out_q16=sign*(int)(whole*65536U + (frac*65536U)/scale);
    return 1;
}

int sv89_execute(SpriteVerbs89 *ctx, const char *target_name, const char *verb, const char *argument)
{
    int op;
    sa89_id p;
    sa89_id a;
    sa89_id c;
    char asset[SV89_ARG_CAP];
    char clip[SV89_ARG_CAP];
    long n;
    char *endp;
    int q16;
    if(!ctx||!ctx->assets||!target_name||!verb)return 0;
    op=sv89_resolve_verb(verb);if(op==SV89_VERB_UNKNOWN){ctx->last_error=SV89_ERR_COMMAND;return 0;}
    p=sv89_bind_target(ctx,target_name);if(p==SA89_INVALID_ID){ctx->last_error=SV89_ERR_TARGET;return 0;}
    if(op==SV89_VERB_HIDE){sa89_player_show(ctx->assets,p,0);return 1;}
    if(op==SV89_VERB_STOP){sa89_player_stop(ctx->assets,p);return 1;}
    if(op==SV89_VERB_RESET){if(ctx->assets->players[p].clip_id!=SA89_INVALID_ID)sa89_player_set_frame(ctx->assets,p,0U);return 1;}
    if(op==SV89_VERB_FLIP_X){ctx->assets->players[p].draw_flags^=SA89_DRAW_FLIP_X;return 1;}
    if(op==SV89_VERB_FLIP_Y){ctx->assets->players[p].draw_flags^=SA89_DRAW_FLIP_Y;return 1;}
    if(op==SV89_VERB_SET_X){if(!argument)return 0;n=strtol(argument,&endp,10);if(*endp)return 0;ctx->assets->players[p].x=(sa89_s32)n;return 1;}
    if(op==SV89_VERB_SET_Y){if(!argument)return 0;n=strtol(argument,&endp,10);if(*endp)return 0;ctx->assets->players[p].y=(sa89_s32)n;return 1;}
    if(op==SV89_VERB_SET_POS){
        long yv;
        const char *q;
        if(!argument)return 0;
        n=strtol(argument,&endp,10);
        if(endp==argument)return 0;
        q=endp;while(*q==' '||*q=='\t'||*q==',')++q;
        if(!*q)return 0;
        yv=strtol(q,&endp,10);if(*endp)return 0;
        sa89_player_set_position(ctx->assets,p,(sa89_s32)n,(sa89_s32)yv);return 1;
    }
    if(op==SV89_VERB_SET_FRAME){if(!argument)return 0;n=strtol(argument,&endp,10);if(*endp||n<0L||n>65534L)return 0;return sa89_player_set_frame(ctx->assets,p,(sa89_id)n);}
    if(op==SV89_VERB_SET_SPEED){if(!sv89_parse_q16(argument,&q16))return 0;sa89_player_set_speed_q16(ctx->assets,p,q16);return 1;}
    sv89_split_asset_clip(argument,asset,SV89_ARG_CAP,clip,SV89_ARG_CAP);
    a=sv89_resolve_asset(ctx,asset);if(a==SA89_INVALID_ID){ctx->last_error=SV89_ERR_ASSET;return 0;}
    if(op==SV89_VERB_SET_ASSET){return sa89_player_set_asset(ctx->assets,p,a);}
    if(op==SV89_VERB_SHOW){
        if(clip[0]) {c=sa89_find_clip(ctx->assets,a,clip);if(c==SA89_INVALID_ID)return 0;ctx->assets->players[p].asset_id=a;ctx->assets->players[p].clip_id=c;ctx->assets->players[p].frame_pos=0U;}
        else if(!sa89_player_set_asset(ctx->assets,p,a))return 0;
        sa89_player_show(ctx->assets,p,1);return 1;
    }
    if(op==SV89_VERB_PLAY){return sa89_player_play(ctx->assets,p,a,clip[0]?clip:0);}
    return 0;
}

int sv89_execute_tokens(SpriteVerbs89 *ctx, int argc, const char **argv)
{
    if(argc<2||!argv)return 0;
    return sv89_execute(ctx,argv[0],argv[1],argc>=3?argv[2]:0);
}
