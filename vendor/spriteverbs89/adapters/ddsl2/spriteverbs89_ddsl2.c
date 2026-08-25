#include "spriteverbs89_ddsl2.h"

static int sv89_d_call(void *user,const ddsl_value *args,int argc,ddsl_value *out,const char *verb)
{
    SpriteVerbs89 *v;
    char target[SV89_TARGET_CAP];
    char arg[SV89_ARG_CAP];
    int ok;
    v=(SpriteVerbs89*)user;
    if(!v||argc<1){if(out)*out=ddsl_v_bool(0);return 0;}
    if(ddsl_value_to_cstr(args[0],target,SV89_TARGET_CAP)<=0){if(out)*out=ddsl_v_bool(0);return 0;}
    arg[0]='\0';
    if(argc>=2)(void)ddsl_value_to_cstr(args[1],arg,SV89_ARG_CAP);
    ok=sv89_execute(v,target,verb,argc>=2?arg:0);
    if(out)*out=ddsl_v_bool(ok);
    return ok;
}
static int sv89_d_show(void*u,const ddsl_value*a,int n,ddsl_value*o){return sv89_d_call(u,a,n,o,"sprite_show");}
static int sv89_d_hide(void*u,const ddsl_value*a,int n,ddsl_value*o){return sv89_d_call(u,a,n,o,"sprite_hide");}
static int sv89_d_play(void*u,const ddsl_value*a,int n,ddsl_value*o){return sv89_d_call(u,a,n,o,"sprite_play");}
static int sv89_d_stop(void*u,const ddsl_value*a,int n,ddsl_value*o){return sv89_d_call(u,a,n,o,"sprite_stop");}
static int sv89_d_frame(void*u,const ddsl_value*a,int n,ddsl_value*o){return sv89_d_call(u,a,n,o,"sprite_frame");}
static int sv89_d_speed(void*u,const ddsl_value*a,int n,ddsl_value*o){return sv89_d_call(u,a,n,o,"sprite_speed");}
int sv89_ddsl2_register(ddsl_registry*r,SpriteVerbs89*v)
{
    if(!r||!v)return 0;
    if(!ddsl_registry_add(r,"sprite_show",sv89_d_show,v))return 0;
    if(!ddsl_registry_add(r,"sprite_hide",sv89_d_hide,v))return 0;
    if(!ddsl_registry_add(r,"sprite_play",sv89_d_play,v))return 0;
    if(!ddsl_registry_add(r,"sprite_stop",sv89_d_stop,v))return 0;
    if(!ddsl_registry_add(r,"sprite_frame",sv89_d_frame,v))return 0;
    if(!ddsl_registry_add(r,"sprite_speed",sv89_d_speed,v))return 0;
    return 1;
}
