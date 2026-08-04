#include <stdio.h>
#define DEMO_WAV_MONO_ONLY 1
#include "demo_wav.h"
#include "gweaponbody89.h"
#include "gmuzzlegas89.h"
#include "gballisticcrack89.h"
#include "glatetail89.h"
#include "gcinemathump89.h"

#define RATE 44100U
#define SECONDS 24U
#define FRAMES (RATE * SECONDS)

static signed short full_mix[FRAMES];
static signed short stem_body[FRAMES];
static signed short stem_gas[FRAMES];
static signed short stem_crack[FRAMES];
static signed short stem_tail[FRAMES];
static signed short stem_thump[FRAMES];

static signed short sat16(signed int x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (signed short)x;
}

static unsigned int rng_next(unsigned int *s)
{
    unsigned int x=*s; if(!x) x=1U; x^=x<<13; x^=x>>17; x^=x<<5; *s=x; return x;
}

static signed short base_shot(unsigned long p, unsigned int *rng, unsigned int kind)
{
    unsigned long len;
    signed int n;
    signed int env;
    signed int body;
    len = kind == 0U ? 9500UL : (kind == 1U ? 12500UL : 15500UL);
    if (p >= len) return 0;
    n = (signed int)((rng_next(rng)>>16)&65535U)-32768;
    env = (signed int)(((len-p)*32767UL)/len);
    body = (n * env) >> 15;
    if (p < 18UL) body += (signed int)((18UL-p)*1250UL);
    if (kind == 1U) body += (signed int)(((signed long)((rng_next(rng)>>16)&65535U)-32768L) * env >> 17);
    if (kind == 2U) body += (signed int)(((signed long)((rng_next(rng)>>16)&65535U)-32768L) * env >> 16);
    return sat16(body);
}

static void mix_at(unsigned long start, unsigned int kind,
                   gwb89_preset_id body_id, gmg89_preset_id gas_id,
                   gbc89_preset_id crack_id, glt89_preset_id tail_id,
                   gct89_preset_id thump_id, int layers)
{
    gwb89_preset bp; gmg89_preset gp; gbc89_preset cp; glt89_preset lp; gct89_preset tp;
    gwb89_context b; gmg89_context g; gbc89_context c; glt89_context l; gct89_context t;
    unsigned long i; unsigned long local; unsigned int rng;
    signed short dry; signed short body; signed short gas; signed short crack; signed short thump; signed short tail;
    signed int mix;
    gwb89_get_preset(body_id,&bp); gmg89_get_preset(gas_id,&gp); gbc89_get_preset(crack_id,&cp);
    glt89_get_preset(tail_id,&lp); gct89_get_preset(thump_id,&tp);
    gwb89_init(&b,RATE,&bp,101U+kind); gmg89_init(&g,RATE,&gp,201U+kind);
    gbc89_init(&c,RATE,&cp,301U+kind); glt89_init(&l,RATE,&lp); gct89_init(&t,RATE,&tp,401U+kind);
    gwb89_trigger(&b,28000,501U+kind); gmg89_trigger(&g,29000,601U+kind);
    gbc89_trigger(&c, layers ? 24U : 0U, 28500,701U+kind); gct89_trigger(&t,28000,801U+kind);
    rng=901U+kind;
    for(i=start;i<FRAMES;++i) {
        local=i-start;
        dry=base_shot(local,&rng,kind);
        body=layers ? gwb89_process_sample(&b,dry) : dry;
        gas=layers ? gmg89_process_sample(&g) : 0;
        crack=layers ? gbc89_process_sample(&c) : 0;
        thump=layers ? gct89_process_sample(&t) : 0;
        mix=(signed int)dry + ((signed int)(body-dry)*3)/4 + ((signed int)gas*3)/5 +
            ((signed int)crack*3)/5 + ((signed int)thump*3)/4;
        tail=layers ? glt89_process_sample(&l,sat16(mix>>1)) : 0;
        mix += layers ? ((signed int)(tail - sat16(mix>>1))*3)/4 : 0;
        full_mix[i]=sat16((signed int)full_mix[i] + (mix*3)/5);
        stem_body[i]=sat16((signed int)stem_body[i] + (body-dry));
        stem_gas[i]=sat16((signed int)stem_gas[i] + gas);
        stem_crack[i]=sat16((signed int)stem_crack[i] + crack);
        stem_thump[i]=sat16((signed int)stem_thump[i] + thump);
        stem_tail[i]=sat16((signed int)stem_tail[i] + (tail - sat16(mix>>1)));
        if(local > RATE*5UL && !gwb89_is_active(&b) && !gmg89_is_active(&g) &&
           !gbc89_is_active(&c) && !gct89_is_active(&t) && !glt89_is_active(&l)) break;
    }
}

int main(void)
{
    unsigned long i;
    for(i=0UL;i<FRAMES;++i) full_mix[i]=stem_body[i]=stem_gas[i]=stem_crack[i]=stem_tail[i]=stem_thump[i]=0;
    mix_at(RATE*1UL,0U,GWB89_PRESET_PISTOL,GMG89_PRESET_PISTOL,GBC89_PRESET_RIFLE_PASS,GLT89_PRESET_SMALL_ROOM,GCT89_PRESET_SUBTLE,0);
    mix_at(RATE*4UL,0U,GWB89_PRESET_PISTOL,GMG89_PRESET_PISTOL,GBC89_PRESET_RIFLE_PASS,GLT89_PRESET_SMALL_ROOM,GCT89_PRESET_ACTION,1);
    mix_at(RATE*8UL,1U,GWB89_PRESET_SHOTGUN,GMG89_PRESET_SHOTGUN,GBC89_PRESET_NEAR,GLT89_PRESET_WAREHOUSE,GCT89_PRESET_SHOTGUN,1);
    mix_at(RATE*13UL,2U,GWB89_PRESET_SNIPER,GMG89_PRESET_SNIPER,GBC89_PRESET_SNIPER_PASS,GLT89_PRESET_EXTERIOR,GCT89_PRESET_SNIPER,1);
    mix_at(RATE*18UL,2U,GWB89_PRESET_LAUNCHER,GMG89_PRESET_LAUNCHER,GBC89_PRESET_MEDIUM,GLT89_PRESET_TUNNEL,GCT89_PRESET_LAUNCHER,1);
    if(!dw_write("audio/00_full_demo.wav",full_mix,FRAMES,RATE)) return 1;
    if(!dw_write("audio/01_body_stem.wav",stem_body,FRAMES,RATE)) return 1;
    if(!dw_write("audio/02_muzzlegas_stem.wav",stem_gas,FRAMES,RATE)) return 1;
    if(!dw_write("audio/03_ballisticcrack_stem.wav",stem_crack,FRAMES,RATE)) return 1;
    if(!dw_write("audio/04_latetail_stem.wav",stem_tail,FRAMES,RATE)) return 1;
    if(!dw_write("audio/05_cinemathump_stem.wav",stem_thump,FRAMES,RATE)) return 1;
    printf("Rendered five-layer procedural demo.\n");
    return 0;
}
