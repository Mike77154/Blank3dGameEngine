#include "wsoundoutdoor89.h"
static wsound89_i16 sat(wsound89_i32 v){if(v>32767)return 32767;if(v<-32768)return -32768;return(wsound89_i16)v;}
wsound89_u32 wsoundoutdoor89_required_frames(wsound89_u32 r,wsoundoutdoor89_preset p){static const wsound89_u16 ms[4]={100,260,230,720};if(!r||(wsound89_u32)p>3U)return 0;return(r*ms[(wsound89_u16)p])/1000U+2U;}
wsound89_result wsoundoutdoor89_init(wsoundoutdoor89_context*c,wsound89_i16*m,wsound89_u32 cap,wsound89_u32 r,wsoundoutdoor89_preset p){
    static const wsound89_u16 ms[4][8]={
        {12,28,53,88,0,0,0,0},
        {19,43,71,116,169,238,0,0},
        {9,21,38,62,97,151,213,0},
        {84,171,278,397,532,681,0,0}
    };
    static const wsound89_i16 gn[4][8]={
        {8000,4800,2800,1600,0,0,0,0},
        {6000,4400,3200,2300,1600,1000,0,0},
        {6200,5000,4000,3100,2300,1600,1000,0},
        {9000,6800,5000,3600,2500,1700,0,0}
    };
    static const wsound89_u8 tc[4]={4,6,7,6};
    wsound89_u32 i,need;
    if(!c||!m||!r||(wsound89_u32)p>3U)return WSOUND89_EINVAL;
    need=wsoundoutdoor89_required_frames(r,p);
    if(cap<need)return WSOUND89_ECAPACITY;
    c->memory=m;c->capacity=cap;c->index=0;c->taps=tc[(wsound89_u16)p];
    for(i=0;i<cap;i++)m[i]=0;
    for(i=0;i<WSOUNDOUTDOOR89_TAPS;i++){
        c->delay[i]=(r*ms[(wsound89_u16)p][(wsound89_u16)i])/1000U;
        c->gain_q15[i]=gn[(wsound89_u16)p][(wsound89_u16)i];
        c->low[i]=0;
    }
    return WSOUND89_OK;
}
static wsound89_i16 process_core(wsoundoutdoor89_context*c,wsound89_i16 in,wsound89_u8 include_dry){
    wsound89_i32 sum,d;
    wsound89_u16 i,shift;
    wsound89_u32 pos;
    if(!c||!c->memory)return include_dry?in:0;
    c->memory[c->index]=in;
    sum=include_dry?in:0;
    for(i=0;i<c->taps;i++){
        pos=(c->index+c->capacity-c->delay[i])%c->capacity;
        d=c->memory[pos];
        shift=(wsound89_u16)(4U+(i>>1));
        c->low[i]+=(d-c->low[i])>>shift;
        sum+=(c->low[i]*c->gain_q15[i])>>15;
    }
    c->index++;
    if(c->index>=c->capacity)c->index=0;
    return sat(sum);
}
wsound89_i16 wsoundoutdoor89_process_sample(wsoundoutdoor89_context*c,wsound89_i16 in){return process_core(c,in,1U);}
wsound89_i16 wsoundoutdoor89_process_wet_sample(wsoundoutdoor89_context*c,wsound89_i16 in){return process_core(c,in,0U);}
