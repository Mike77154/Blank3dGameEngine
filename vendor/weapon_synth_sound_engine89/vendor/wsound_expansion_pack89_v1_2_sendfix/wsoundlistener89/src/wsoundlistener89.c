#include "wsoundlistener89.h"
static wsound89_i16 sat(wsound89_i32 v){if(v>32767)return 32767;if(v<-32768)return -32768;return(wsound89_i16)v;}
static wsound89_i16 ring_wave(wsound89_u32 p){
    static const wsound89_i16 s[32]={
        0,6393,12539,18204,23170,27245,30273,32137,
        32767,32137,30273,27245,23170,18204,12539,6393,
        0,-6393,-12539,-18204,-23170,-27245,-30273,-32137,
        -32767,-32137,-30273,-27245,-23170,-18204,-12539,-6393
    };
    return s[(p>>11)&31U];
}
wsound89_result wsoundlistener89_init(wsoundlistener89_context*c,wsound89_u32 rate){if(!c||!rate)return WSOUND89_EINVAL;c->exposure_q15=0;c->gain_q15=32767;c->low_l=c->low_r=0;c->ring_env=0;c->phase=0;c->phase_inc=(6200U*65536U)/rate;c->recovery_q15=32755;return WSOUND89_OK;}
void wsoundlistener89_expose(wsoundlistener89_context*c,wsound89_u16 pressure,wsound89_u16 distance,wsoundlistener89_protection p){wsound89_i32 e,prot,ring_add;if(!c)return;prot=(p==WSOUNDLISTENER89_NONE)?32767:(p==WSOUNDLISTENER89_EARPLUGS?19000:12000);e=((wsound89_i32)pressure*prot)>>15;e=(e*32767)/(32767+(wsound89_i32)distance);c->exposure_q15+=e;if(c->exposure_q15>32767)c->exposure_q15=32767;ring_add=0;if(p==WSOUNDLISTENER89_NONE)ring_add=e>>4;else if(p==WSOUNDLISTENER89_EARPLUGS)ring_add=e>>8;c->ring_env+=ring_add;if(c->ring_env>3200)c->ring_env=3200;}
void wsoundlistener89_process_stereo(wsoundlistener89_context*c,wsound89_i16 l,wsound89_i16 r,wsound89_i16*ol,wsound89_i16*or){wsound89_i32 target,ring;if(!c||!ol||!or)return;target=32767-(c->exposure_q15>>1);c->gain_q15+=(target-c->gain_q15)>>7;c->low_l+=(l-c->low_l)>>(2+(c->exposure_q15>>14));c->low_r+=(r-c->low_r)>>(2+(c->exposure_q15>>14));c->phase+=c->phase_inc;ring=(ring_wave(c->phase)*c->ring_env)>>15;*ol=sat(((c->low_l*c->gain_q15)>>15)+ring);*or=sat(((c->low_r*c->gain_q15)>>15)+ring);c->exposure_q15=(c->exposure_q15*c->recovery_q15)>>15;c->ring_env=(c->ring_env*32762)>>15;}
