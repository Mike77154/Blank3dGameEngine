#ifndef GFIRE89_FX_H
#define GFIRE89_FX_H

#include "gfire89.h"

/*
    Optional source-space and room processors.

    gfire89_spread:
      short, slowly wandering decorrelation delays;
      a restrained chorus-like spatializer without feedback.

    gfire89_room:
      one global room bus using four damped delay lines and
      two diffusion allpasses. Do not create one room per voice.
*/

#define GFIRE89_SPREAD_DELAY_SAMPLES 1024

#define GFIRE89_ROOM_COMB0_SAMPLES 1093
#define GFIRE89_ROOM_COMB1_SAMPLES 1301
#define GFIRE89_ROOM_COMB2_SAMPLES 1559
#define GFIRE89_ROOM_COMB3_SAMPLES 1789
#define GFIRE89_ROOM_APL_SAMPLES 211
#define GFIRE89_ROOM_APR_SAMPLES 337

typedef struct gfire89_spread_s {
    gfire89_s16 delay[GFIRE89_SPREAD_DELAY_SAMPLES];
    gfire89_s32 write_index;
    gfire89_s32 mod_q8;
    gfire89_s32 target_q8;
    gfire89_s32 control_countdown;
    gfire89_s32 lp_left;
    gfire89_s32 lp_right;
    gfire89_s32 wet_q15;
    gfire89_s32 width_q15;
    gfire89_u32 rng;
} gfire89_spread;

typedef struct gfire89_room_s {
    gfire89_s16 comb0[GFIRE89_ROOM_COMB0_SAMPLES];
    gfire89_s16 comb1[GFIRE89_ROOM_COMB1_SAMPLES];
    gfire89_s16 comb2[GFIRE89_ROOM_COMB2_SAMPLES];
    gfire89_s16 comb3[GFIRE89_ROOM_COMB3_SAMPLES];
    gfire89_s16 allpass_left[GFIRE89_ROOM_APL_SAMPLES];
    gfire89_s16 allpass_right[GFIRE89_ROOM_APR_SAMPLES];

    gfire89_s32 index0;
    gfire89_s32 index1;
    gfire89_s32 index2;
    gfire89_s32 index3;
    gfire89_s32 index_ap_left;
    gfire89_s32 index_ap_right;

    gfire89_s32 damp0;
    gfire89_s32 damp1;
    gfire89_s32 damp2;
    gfire89_s32 damp3;

    gfire89_s32 decay_q15;
    gfire89_s32 damping_q15;
    gfire89_s32 wet_q15;
} gfire89_room;

void gfire89_spread_init(gfire89_spread *spread, gfire89_u32 seed);
void gfire89_spread_set(
    gfire89_spread *spread,
    gfire89_s32 wet_q15,
    gfire89_s32 width_q15
);
void gfire89_spread_process(
    gfire89_spread *spread,
    gfire89_s16 mono,
    gfire89_s16 *left,
    gfire89_s16 *right
);

void gfire89_room_init(gfire89_room *room);
void gfire89_room_set(
    gfire89_room *room,
    gfire89_s32 wet_q15,
    gfire89_s32 decay_q15,
    gfire89_s32 damping_q15
);
void gfire89_room_process(
    gfire89_room *room,
    gfire89_s16 mono,
    gfire89_s16 *wet_left,
    gfire89_s16 *wet_right
);

#endif
