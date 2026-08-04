#include <stdio.h>
#define DEMO_WAV_STEREO_ONLY 1
#include "demo_wav.h"
#include "gweaponbody89.h"
#include "gmuzzlegas89.h"
#include "gballisticcrack89.h"
#include "glatetail89.h"
#include "gcinemathump89.h"

#define MAX_FRAMES (44100UL * 20UL)
#define MAX_SAMPLES (MAX_FRAMES * 2UL)
static signed short source_pcm[MAX_SAMPLES];
static signed short output_pcm[MAX_SAMPLES];
static signed short cushion_stem[MAX_SAMPLES];
static gwb89_context body_ctx;
static gmg89_context gas_ctx;
static gbc89_context crack_ctx;
static glt89_context tail_ctx;
static gct89_context thump_ctx;

static unsigned long rd_u32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

static unsigned int rd_u16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static signed short sat16(signed int x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (signed short)x;
}

static int read_pcm16_mono(const char *path, unsigned int *rate,
                           unsigned long *frames)
{
    FILE *f;
    unsigned char h[44];
    unsigned long bytes;
    unsigned long i;
    f = fopen(path, "rb");
    if (f == 0) return 0;
    if (fread(h, 1U, 44U, f) != 44U) { fclose(f); return 0; }
    if (h[0] != 'R' || h[1] != 'I' || h[2] != 'F' || h[3] != 'F') { fclose(f); return 0; }
    if (rd_u16(h + 20) != 1U || rd_u16(h + 22) != 2U || rd_u16(h + 34) != 16U) { fclose(f); return 0; }
    *rate = (unsigned int)rd_u32(h + 24);
    bytes = rd_u32(h + 40);
    *frames = bytes / 4UL;
    if (*frames > MAX_FRAMES) { fclose(f); return 0; }
    for (i = 0UL; i < *frames * 2UL; ++i) {
        unsigned char b[2];
        unsigned int u;
        if (fread(b, 1U, 2U, f) != 2U) { fclose(f); return 0; }
        u = rd_u16(b);
        source_pcm[i] = (signed short)u;
    }
    fclose(f);
    return 1;
}

int main(int argc, char **argv)
{
    gwb89_preset bp;
    gmg89_preset gp;
    gbc89_preset cp;
    glt89_preset lp;
    gct89_preset tp;
    unsigned int rate;
    unsigned long frames;
    unsigned long shot_a;
    unsigned long shot_b;
    unsigned long i;
    signed short body;
    signed short gas;
    signed short crack;
    signed short thump;
    signed short tail;
    signed short feed;
    signed short mono;
    signed int cushion;
    signed int mix;
    const char *input_path;
    const char *output_path;

    if (argc < 3) {
        fprintf(stderr, "usage: augment_sequence input.wav output.wav\n");
        return 1;
    }
    input_path = argv[1];
    output_path = argv[2];
    if (!read_pcm16_mono(input_path, &rate, &frames)) return 2;
    if (rate == 0U || rate > 48000U) return 3;

    gwb89_get_preset(GWB89_PRESET_SHOTGUN, &bp);
    gmg89_get_preset(GMG89_PRESET_SHOTGUN, &gp);
    gbc89_get_preset(GBC89_PRESET_NEAR, &cp);
    glt89_get_preset(GLT89_PRESET_WAREHOUSE, &lp);
    gct89_get_preset(GCT89_PRESET_SHOTGUN, &tp);
    bp.dry_q15 = 0;
    bp.wet_q15 = 29500;
    lp.dry_q15 = 0;
    lp.wet_q15 = 24500;
    lp.input_gain_q15 = 18500;

    if (!gwb89_init(&body_ctx, rate, &bp, 11U)) return 4;
    if (!gmg89_init(&gas_ctx, rate, &gp, 12U)) return 4;
    if (!gbc89_init(&crack_ctx, rate, &cp, 13U)) return 4;
    if (!glt89_init(&tail_ctx, rate, &lp)) return 4;
    if (!gct89_init(&thump_ctx, rate, &tp, 14U)) return 4;

    shot_a = ((unsigned long)rate * 203UL) / 100UL;
    shot_b = ((unsigned long)rate * 365UL) / 100UL;

    for (i = 0UL; i < frames; ++i) {
        if (i == shot_a || i == shot_b) {
            unsigned int salt;
            salt = i == shot_a ? 100U : 200U;
            gwb89_trigger(&body_ctx, 30000, 1000U + salt);
            gmg89_trigger(&gas_ctx, 30000, 2000U + salt);
            gbc89_trigger(&crack_ctx, 18U, 29000, 3000U + salt);
            gct89_trigger(&thump_ctx, 28500, 4000U + salt);
        }
        mono = sat16(((signed int)source_pcm[i * 2UL] + (signed int)source_pcm[i * 2UL + 1UL]) >> 1);
        body = gwb89_process_sample(&body_ctx, mono);
        gas = gmg89_process_sample(&gas_ctx);
        crack = gbc89_process_sample(&crack_ctx);
        thump = gct89_process_sample(&thump_ctx);
        feed = sat16(((signed int)mono >> 1) + ((signed int)body >> 2) +
                     ((signed int)gas >> 2) + ((signed int)crack >> 2));
        tail = glt89_process_sample(&tail_ctx, feed);
        cushion = ((signed int)body * 9) / 32 +
                  ((signed int)gas * 10) / 32 +
                  ((signed int)crack * 12) / 32 +
                  ((signed int)thump * 14) / 32 +
                  ((signed int)tail * 13) / 32;
        cushion_stem[i * 2UL] = sat16(cushion);
        cushion_stem[i * 2UL + 1UL] = sat16(cushion);
        mix = (signed int)source_pcm[i * 2UL] + cushion / 2;
        output_pcm[i * 2UL] = sat16(mix);
        mix = (signed int)source_pcm[i * 2UL + 1UL] + cushion / 2;
        output_pcm[i * 2UL + 1UL] = sat16(mix);
    }

    if (!dw_write_stereo(output_path, output_pcm, frames, rate)) return 5;
    if (!dw_write_stereo("audio/06_cushions_over_sequence_stem.wav", cushion_stem, frames, rate)) return 6;
    printf("Augmented %lu frames at %u Hz.\n", frames, rate);
    return 0;
}
