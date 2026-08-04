#include "gshotgunsequence89.h"

#include <stdio.h>
#include <string.h>

#define SMS_RATE 44100U
#define SMS_AB_SECONDS 6U
#define SMS_MODELS_SECONDS 10U
#define SMS_FULL_SECONDS 7U
#define SMS_AB_FRAMES (SMS_RATE * SMS_AB_SECONDS)
#define SMS_MODELS_FRAMES (SMS_RATE * SMS_MODELS_SECONDS)
#define SMS_FULL_FRAMES (SMS_RATE * SMS_FULL_SECONDS)
#define SMS_MAX_RENDER_FRAMES 52920U

static gss89_s16 sms_ab[SMS_AB_FRAMES * 2U];
static gss89_s16 sms_models[SMS_MODELS_FRAMES * 2U];
static gss89_s16 sms_full[SMS_FULL_FRAMES * 2U];
static gss89_s16 sms_mono[SMS_MAX_RENDER_FRAMES];
static gss89_s16 sms_texture0[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 sms_texture1[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_context sms_ctx;

static gss89_s16 sms_sat16(gss89_s32 value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (gss89_s16)value;
}

static void sms_put_u16le(FILE *file, unsigned int value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
}

static void sms_put_u32le(FILE *file, unsigned int value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
    fputc((int)((value >> 16) & 255U), file);
    fputc((int)((value >> 24) & 255U), file);
}

static int sms_write_wav(const char *path, const gss89_s16 *stereo,
                         unsigned int frames)
{
    FILE *file;
    unsigned int i;
    unsigned int samples;
    unsigned int data_bytes;

    file = fopen(path, "wb");
    if (file == 0) return 0;
    samples = frames * 2U;
    data_bytes = samples * 2U;

    fwrite("RIFF", 1U, 4U, file);
    sms_put_u32le(file, 36U + data_bytes);
    fwrite("WAVE", 1U, 4U, file);
    fwrite("fmt ", 1U, 4U, file);
    sms_put_u32le(file, 16U);
    sms_put_u16le(file, 1U);
    sms_put_u16le(file, 2U);
    sms_put_u32le(file, SMS_RATE);
    sms_put_u32le(file, SMS_RATE * 4U);
    sms_put_u16le(file, 4U);
    sms_put_u16le(file, 16U);
    fwrite("data", 1U, 4U, file);
    sms_put_u32le(file, data_bytes);

    for (i = 0U; i < samples; ++i) {
        sms_put_u16le(file, (unsigned int)(unsigned short)stereo[i]);
    }
    fclose(file);
    return 1;
}

static int sms_setup(unsigned int seed)
{
    if (gss89_init(&sms_ctx, SMS_RATE, seed) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&sms_ctx, 0U, sms_texture0,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&sms_ctx, 1U, sms_texture1,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    return 1;
}

static void sms_pump_only_mix(gss89_context *ctx)
{
    gss89_mix mix;
    mix = ctx->mix;
    mix.report_q15 = 0;
    mix.report_body_q15 = 0;
    mix.report_gas_q15 = 0;
    mix.report_crack_q15 = 0;
    mix.report_thump_q15 = 0;
    mix.report_tail_q15 = 0;
    mix.general_chuecka_q15 = 0;
    mix.foley_q15 = 0;
    gss89_set_mix(ctx, &mix);
}

static int sms_render_model(gss89_s16 *dst, unsigned int dst_frames,
                            unsigned int start_frame,
                            gss89_shotgun_model model,
                            unsigned int seed,
                            int master_enabled,
                            int pump_only)
{
    unsigned int i;
    unsigned int frame;
    gss89_s32 value;

    if (!sms_setup(seed)) return 0;
    gss89_set_shotgun_model(&sms_ctx, model);
    if (pump_only) sms_pump_only_mix(&sms_ctx);
    if (gss89_trigger_model_pump(&sms_ctx, model, seed + 1U) != GSS89_OK) {
        return 0;
    }
    gss89_enable_pump_master(&sms_ctx, master_enabled);
    memset(sms_mono, 0, sizeof(sms_mono));
    gss89_render_mono(&sms_ctx, sms_mono, SMS_MAX_RENDER_FRAMES);

    for (i = 0U; i < SMS_MAX_RENDER_FRAMES; ++i) {
        frame = start_frame + i;
        if (frame >= dst_frames) break;
        value = (gss89_s32)dst[frame * 2U] + (gss89_s32)sms_mono[i];
        dst[frame * 2U] = sms_sat16(value);
        dst[frame * 2U + 1U] = dst[frame * 2U];
    }
    return 1;
}

static int sms_render_ab(void)
{
    memset(sms_ab, 0, sizeof(sms_ab));
    /* Same balanced mechanism and seed: raw triple layer, then shotgun master. */
    if (!sms_render_model(sms_ab, SMS_AB_FRAMES, 22050U,
        GSS89_SHOTGUN_MODEL_BALANCED, 7716201U, 0, 1)) return 0;
    if (!sms_render_model(sms_ab, SMS_AB_FRAMES, 154350U,
        GSS89_SHOTGUN_MODEL_BALANCED, 7716201U, 1, 1)) return 0;
    return sms_write_wav("audio/wsse89_v1_6_2_shotgun_master_ab.wav",
                         sms_ab, SMS_AB_FRAMES);
}

static int sms_render_models(void)
{
    memset(sms_models, 0, sizeof(sms_models));
    if (!sms_render_model(sms_models, SMS_MODELS_FRAMES, 17640U,
        GSS89_SHOTGUN_MODEL_REMINGTON_870, 7716210U, 1, 1)) return 0;
    if (!sms_render_model(sms_models, SMS_MODELS_FRAMES, 110250U,
        GSS89_SHOTGUN_MODEL_MOSSBERG_500_590, 7716211U, 1, 1)) return 0;
    if (!sms_render_model(sms_models, SMS_MODELS_FRAMES, 202860U,
        GSS89_SHOTGUN_MODEL_BENELLI_NOVA, 7716212U, 1, 1)) return 0;
    if (!sms_render_model(sms_models, SMS_MODELS_FRAMES, 295470U,
        GSS89_SHOTGUN_MODEL_WINCHESTER_SXP, 7716213U, 1, 1)) return 0;
    return sms_write_wav("audio/wsse89_v1_6_2_shotgun_model_pumps.wav",
                         sms_models, SMS_MODELS_FRAMES);
}

static int sms_render_full(void)
{
    unsigned int i;
    gss89_s16 sample;
    gss89_s32 left;
    gss89_s32 right;

    memset(sms_full, 0, sizeof(sms_full));
    if (!sms_setup(7716220U)) return 0;
    gss89_set_shotgun_model(&sms_ctx, GSS89_SHOTGUN_MODEL_REMINGTON_870);

    for (i = 0U; i < SMS_FULL_FRAMES; ++i) {
        if (i == 22050U) gss89_trigger_report(&sms_ctx, 7716221U);
        if (i == 61740U) {
            if (gss89_trigger_model_pump(&sms_ctx,
                GSS89_SHOTGUN_MODEL_REMINGTON_870, 7716222U) != GSS89_OK) {
                return 0;
            }
        }
        if (i == 154350U) gss89_trigger_report(&sms_ctx, 7716223U);
        if (i == 190512U) {
            if (gss89_trigger_model_pump(&sms_ctx,
                GSS89_SHOTGUN_MODEL_WINCHESTER_SXP, 7716224U) != GSS89_OK) {
                return 0;
            }
        }
        sample = gss89_process_sample(&sms_ctx);
        left = (gss89_s32)sample;
        right = (gss89_s32)sample;
        if (i < 132300U) {
            left = (left * 30000) / 32768;
            right = (right * 26000) / 32768;
        } else {
            left = (left * 26000) / 32768;
            right = (right * 30000) / 32768;
        }
        sms_full[i * 2U] = sms_sat16(left);
        sms_full[i * 2U + 1U] = sms_sat16(right);
    }
    return sms_write_wav(
        "audio/wsse89_v1_6_2_full_shotgun_sequence_mastered.wav",
        sms_full, SMS_FULL_FRAMES);
}

int main(void)
{
    if (!sms_render_ab()) {
        fprintf(stderr, "could not render shotgun master A/B\n");
        return 1;
    }
    if (!sms_render_models()) {
        fprintf(stderr, "could not render shotgun model comparison\n");
        return 2;
    }
    if (!sms_render_full()) {
        fprintf(stderr, "could not render full mastered shotgun sequence\n");
        return 3;
    }
    printf("wrote shotgun master A/B, model comparison and full sequence\n");
    printf("master context bytes: %lu\n",
           (unsigned long)gsgeq89_context_bytes());
    return 0;
}
