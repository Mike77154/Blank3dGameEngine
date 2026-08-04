#include "gfire89_fx.h"

static gfire89_s32 gfire89_fx_clamp_q15(gfire89_s32 x)
{
    if (x < 0) {
        return 0;
    }
    if (x > 32767) {
        return 32767;
    }
    return x;
}

static gfire89_s32 gfire89_fx_clamp_s16(gfire89_s32 x)
{
    if (x < -32768) {
        return -32768;
    }
    if (x > 32767) {
        return 32767;
    }
    return x;
}

static gfire89_s32 gfire89_fx_mul_q15(gfire89_s32 a, gfire89_s32 b)
{
    gfire89_s32 p;
    p = a * b;
    if (p < 0) {
        return -((-p) / 32768);
    }
    return p / 32768;
}

static gfire89_s32 gfire89_fx_div_pow2(
    gfire89_s32 x,
    gfire89_s32 shift
)
{
    gfire89_s32 d;
    d = (gfire89_s32)(1U << (gfire89_u32)shift);
    if (x < 0) {
        return -((-x) / d);
    }
    return x / d;
}

static gfire89_u32 gfire89_fx_random(gfire89_spread *spread)
{
    spread->rng = spread->rng * 1664525U + 1013904223U;
    return spread->rng;
}

static void gfire89_fx_zero(void *ptr, gfire89_s32 bytes)
{
    gfire89_s32 i;
    unsigned char *dst;
    dst = (unsigned char *)ptr;
    for (i = 0; i < bytes; ++i) {
        dst[i] = 0;
    }
}

static gfire89_s32 gfire89_spread_read(
    gfire89_spread *spread,
    gfire89_s32 delay_q8
)
{
    gfire89_s32 ring_q8;
    gfire89_s32 position_q8;
    gfire89_s32 index0;
    gfire89_s32 index1;
    gfire89_s32 fraction;
    gfire89_s32 a;
    gfire89_s32 b;

    ring_q8 = GFIRE89_SPREAD_DELAY_SAMPLES << 8;
    position_q8 = (spread->write_index << 8) - delay_q8;

    while (position_q8 < 0) {
        position_q8 += ring_q8;
    }
    while (position_q8 >= ring_q8) {
        position_q8 -= ring_q8;
    }

    index0 = position_q8 >> 8;
    index1 = index0 + 1;
    if (index1 >= GFIRE89_SPREAD_DELAY_SAMPLES) {
        index1 = 0;
    }

    fraction = position_q8 & 255;
    a = spread->delay[index0];
    b = spread->delay[index1];

    return a + ((b - a) * fraction) / 256;
}

void gfire89_spread_init(gfire89_spread *spread, gfire89_u32 seed)
{
    if (spread == 0) {
        return;
    }

    gfire89_fx_zero(spread, (gfire89_s32)sizeof(*spread));
    spread->rng = seed;
    if (spread->rng == 0U) {
        spread->rng = 0x91E10DA5U;
    }

    spread->wet_q15 = 4200;
    spread->width_q15 = 32767;
    spread->control_countdown = 1;
}

void gfire89_spread_set(
    gfire89_spread *spread,
    gfire89_s32 wet_q15,
    gfire89_s32 width_q15
)
{
    if (spread == 0) {
        return;
    }

    spread->wet_q15 = gfire89_fx_clamp_q15(wet_q15);
    spread->width_q15 = gfire89_fx_clamp_q15(width_q15);
}

void gfire89_spread_process(
    gfire89_spread *spread,
    gfire89_s16 mono,
    gfire89_s16 *left,
    gfire89_s16 *right
)
{
    gfire89_s32 dry;
    gfire89_s32 tap_left;
    gfire89_s32 tap_right;
    gfire89_s32 delayed_left;
    gfire89_s32 delayed_right;
    gfire89_s32 side;
    gfire89_s32 wet_left;
    gfire89_s32 wet_right;
    gfire89_s32 out_left;
    gfire89_s32 out_right;

    if (spread == 0 || left == 0 || right == 0) {
        return;
    }

    dry = mono;
    spread->delay[spread->write_index] = mono;

    spread->control_countdown -= 1;
    if (spread->control_countdown <= 0) {
        spread->control_countdown = 64;

        if (spread->mod_q8 < spread->target_q8) {
            spread->mod_q8 += 48;
        } else if (spread->mod_q8 > spread->target_q8) {
            spread->mod_q8 -= 48;
        } else {
            spread->target_q8 =
                (gfire89_s32)((gfire89_fx_random(spread) >> 16) & 16383U)
                - 8192;
        }

        if (spread->mod_q8 > 8192) {
            spread->mod_q8 = 8192;
        }
        if (spread->mod_q8 < -8192) {
            spread->mod_q8 = -8192;
        }

        if ((gfire89_fx_random(spread) & 255U) == 0U) {
            spread->target_q8 =
                (gfire89_s32)((gfire89_fx_random(spread) >> 16) & 16383U)
                - 8192;
        }
    }

    tap_left = (360 << 8) + spread->mod_q8;
    tap_right = (620 << 8) - spread->mod_q8;

    delayed_left = gfire89_spread_read(spread, tap_left);
    delayed_right = gfire89_spread_read(spread, tap_right);

    spread->lp_left += gfire89_fx_div_pow2(
        delayed_left - spread->lp_left,
        2
    );
    spread->lp_right += gfire89_fx_div_pow2(
        delayed_right - spread->lp_right,
        2
    );

    delayed_left = spread->lp_left;
    delayed_right = spread->lp_right;

    side = (delayed_left - delayed_right) / 2;
    side = gfire89_fx_mul_q15(side, spread->width_q15);

    wet_left = (delayed_left + delayed_right) / 2 + side;
    wet_right = (delayed_left + delayed_right) / 2 - side;

    out_left = dry + gfire89_fx_mul_q15(
        wet_left - dry,
        spread->wet_q15
    );

    out_right = dry + gfire89_fx_mul_q15(
        wet_right - dry,
        spread->wet_q15
    );

    *left = (gfire89_s16)gfire89_fx_clamp_s16(out_left);
    *right = (gfire89_s16)gfire89_fx_clamp_s16(out_right);

    spread->write_index += 1;
    if (spread->write_index >= GFIRE89_SPREAD_DELAY_SAMPLES) {
        spread->write_index = 0;
    }
}

void gfire89_room_init(gfire89_room *room)
{
    if (room == 0) {
        return;
    }

    gfire89_fx_zero(room, (gfire89_s32)sizeof(*room));
    room->wet_q15 = 5000;
    room->decay_q15 = 23500;
    room->damping_q15 = 17000;
}

void gfire89_room_set(
    gfire89_room *room,
    gfire89_s32 wet_q15,
    gfire89_s32 decay_q15,
    gfire89_s32 damping_q15
)
{
    if (room == 0) {
        return;
    }

    room->wet_q15 = gfire89_fx_clamp_q15(wet_q15);
    room->decay_q15 = gfire89_fx_clamp_q15(decay_q15);
    room->damping_q15 = gfire89_fx_clamp_q15(damping_q15);
}

static gfire89_s32 gfire89_room_comb(
    gfire89_s16 *buffer,
    gfire89_s32 length,
    gfire89_s32 *index,
    gfire89_s32 *damp_state,
    gfire89_s32 input,
    gfire89_s32 decay_q15,
    gfire89_s32 damping_q15
)
{
    gfire89_s32 delayed;
    gfire89_s32 lowpass;
    gfire89_s32 feedback;

    delayed = buffer[*index];
    lowpass = gfire89_fx_mul_q15(
        delayed,
        32767 - damping_q15
    );
    lowpass += gfire89_fx_mul_q15(
        *damp_state,
        damping_q15
    );

    *damp_state = lowpass;
    feedback = input + gfire89_fx_mul_q15(lowpass, decay_q15);
    buffer[*index] = (gfire89_s16)gfire89_fx_clamp_s16(feedback);

    *index += 1;
    if (*index >= length) {
        *index = 0;
    }

    return delayed;
}

static gfire89_s32 gfire89_room_allpass(
    gfire89_s16 *buffer,
    gfire89_s32 length,
    gfire89_s32 *index,
    gfire89_s32 input
)
{
    gfire89_s32 delayed;
    gfire89_s32 output;
    gfire89_s32 stored;

    delayed = buffer[*index];
    output = delayed - input / 2;
    stored = input + delayed / 2;
    buffer[*index] = (gfire89_s16)gfire89_fx_clamp_s16(stored);

    *index += 1;
    if (*index >= length) {
        *index = 0;
    }

    return output;
}

void gfire89_room_process(
    gfire89_room *room,
    gfire89_s16 mono,
    gfire89_s16 *wet_left,
    gfire89_s16 *wet_right
)
{
    gfire89_s32 input;
    gfire89_s32 c0;
    gfire89_s32 c1;
    gfire89_s32 c2;
    gfire89_s32 c3;
    gfire89_s32 left;
    gfire89_s32 right;

    if (room == 0 || wet_left == 0 || wet_right == 0) {
        return;
    }

    input = mono / 2;

    c0 = gfire89_room_comb(
        room->comb0,
        GFIRE89_ROOM_COMB0_SAMPLES,
        &room->index0,
        &room->damp0,
        input,
        room->decay_q15,
        room->damping_q15
    );

    c1 = gfire89_room_comb(
        room->comb1,
        GFIRE89_ROOM_COMB1_SAMPLES,
        &room->index1,
        &room->damp1,
        input,
        room->decay_q15,
        room->damping_q15
    );

    c2 = gfire89_room_comb(
        room->comb2,
        GFIRE89_ROOM_COMB2_SAMPLES,
        &room->index2,
        &room->damp2,
        input,
        room->decay_q15,
        room->damping_q15
    );

    c3 = gfire89_room_comb(
        room->comb3,
        GFIRE89_ROOM_COMB3_SAMPLES,
        &room->index3,
        &room->damp3,
        input,
        room->decay_q15,
        room->damping_q15
    );

    left = (c0 + c2 - c1 / 2 + c3 / 3) / 2;
    right = (c1 + c3 - c0 / 2 + c2 / 3) / 2;

    left = gfire89_room_allpass(
        room->allpass_left,
        GFIRE89_ROOM_APL_SAMPLES,
        &room->index_ap_left,
        left
    );

    right = gfire89_room_allpass(
        room->allpass_right,
        GFIRE89_ROOM_APR_SAMPLES,
        &room->index_ap_right,
        right
    );

    left = gfire89_fx_mul_q15(left, room->wet_q15);
    right = gfire89_fx_mul_q15(right, room->wet_q15);

    *wet_left = (gfire89_s16)gfire89_fx_clamp_s16(left);
    *wet_right = (gfire89_s16)gfire89_fx_clamp_s16(right);
}
