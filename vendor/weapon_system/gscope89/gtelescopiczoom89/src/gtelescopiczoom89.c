#include "../include/gtelescopiczoom89.h"

#define GTZ89_ABS(a) ((a) < 0 ? -(a) : (a))
#define GTZ89_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

static const g89_fx gtz89_tan_deg_q16[90] = {
    0, 1144, 2289, 3435, 4583, 5734,
    6888, 8047, 9210, 10380, 11556, 12739,
    13930, 15130, 16340, 17560, 18792, 20036,
    21294, 22566, 23853, 25157, 26478, 27818,
    29179, 30560, 31964, 33392, 34846, 36327,
    37837, 39378, 40951, 42560, 44205, 45889,
    47615, 49385, 51202, 53070, 54991, 56970,
    59009, 61113, 63287, 65536, 67865, 70279,
    72785, 75391, 78103, 80930, 83882, 86969,
    90203, 93595, 97161, 100917, 104880, 109070,
    113512, 118230, 123255, 128622, 134369, 140542,
    147196, 154393, 162207, 170727, 180059, 190330,
    201699, 214359, 228551, 244584, 262851, 283868,
    308323, 337153, 371673, 413778, 466313, 533748,
    623533, 749080, 937208, 1250501, 1876705, 3754555
};

static g89_fx gtz89_fx_div(g89_fx a, g89_fx b)
{
    if (b == 0) return 0;
    if ((b / 256L) == 0) return 0;
    return (g89_fx)((a * 256L) / (b / 256L));
}

static short gtz89_lerp_short_internal(short a, short b, short t_x1000)
{
    long d;
    d = (long)b - (long)a;
    return (short)((long)a + (d * (long)t_x1000) / 1000L);
}

static g89_fx gtz89_tan_deg_x100(short deg_x100)
{
    short d;
    short frac;
    g89_fx a;
    g89_fx b;

    if (deg_x100 < 0) deg_x100 = (short)-deg_x100;
    if (deg_x100 > 8900) deg_x100 = 8900;
    d = (short)(deg_x100 / 100);
    frac = (short)(deg_x100 - d * 100);
    if (d >= 89) return gtz89_tan_deg_q16[89];
    a = gtz89_tan_deg_q16[d];
    b = gtz89_tan_deg_q16[d + 1];
    return a + (g89_fx)(((b - a) * (g89_fx)frac) / 100L);
}

static short gtz89_atan_q16_x100(g89_fx x)
{
    short i;
    g89_fx best_diff;
    short best;

    if (x < 0) x = -x;
    best = 0;
    best_diff = GTZ89_ABS(x - gtz89_tan_deg_q16[0]);

    for (i = 1; i < 90; ++i) {
        g89_fx diff;
        diff = GTZ89_ABS(x - gtz89_tan_deg_q16[i]);
        if (diff < best_diff) {
            best_diff = diff;
            best = i;
        } else if (gtz89_tan_deg_q16[i] > x && i > 1) {
            break;
        }
    }

    if (best < 89 && gtz89_tan_deg_q16[best] < x) {
        g89_fx lo;
        g89_fx hi;
        g89_fx span;
        g89_fx off;
        lo = gtz89_tan_deg_q16[best];
        hi = gtz89_tan_deg_q16[best + 1];
        span = hi - lo;
        off = x - lo;
        if (span > 0) {
            return (short)(best * 100 + (short)((off * 100L) / span));
        }
    }

    if (best > 0 && gtz89_tan_deg_q16[best] > x) {
        g89_fx lo2;
        g89_fx hi2;
        g89_fx span2;
        g89_fx off2;
        lo2 = gtz89_tan_deg_q16[best - 1];
        hi2 = gtz89_tan_deg_q16[best];
        span2 = hi2 - lo2;
        off2 = x - lo2;
        if (span2 > 0) {
            return (short)((best - 1) * 100 +
                           (short)((off2 * 100L) / span2));
        }
    }

    return (short)(best * 100);
}

short gtz89_solve_fov_deg_x100(short base_fov_deg_x100,
                               short zoom_x100)
{
    short half;
    g89_fx tan_half;
    g89_fx zoom_q16;
    g89_fx scaled;
    short atan_half;

    if (base_fov_deg_x100 < 100) base_fov_deg_x100 = 100;
    if (base_fov_deg_x100 > 17800) base_fov_deg_x100 = 17800;
    if (zoom_x100 < 100) zoom_x100 = 100;

    half = (short)(base_fov_deg_x100 / 2);
    tan_half = gtz89_tan_deg_x100(half);
    zoom_q16 = (g89_fx)(((long)zoom_x100 * 65536L) / 100L);
    scaled = gtz89_fx_div(tan_half, zoom_q16);
    atan_half = gtz89_atan_q16_x100(scaled);
    return (short)(atan_half * 2);
}

void gtz89_provider_init(gtz89_provider *provider)
{
    if (!provider) return;
    provider->mode = GTZ89_PROVIDER_INTERNAL;
    provider->user = 0;
    provider->camera = 0;
    provider->read_base_fov = 0;
    provider->write_fov = 0;
    provider->write_sensitivity = 0;
    provider->solve_fov = 0;
    provider->lerp_short = 0;
}

static short gtz89_provider_camera_enabled(const gtz89_ctx *ctx)
{
    if (!ctx || !ctx->provider_enabled) return 0;
    return (short)((ctx->provider.mode & GTZ89_PROVIDER_CAMERA) != 0u);
}

static short gtz89_provider_math_enabled(const gtz89_ctx *ctx)
{
    if (!ctx || !ctx->provider_enabled) return 0;
    return (short)((ctx->provider.mode & GTZ89_PROVIDER_MATH) != 0u);
}

static short gtz89_provider_read_base_fov(gtz89_ctx *ctx,
                                          short *out_fov_deg_x100)
{
    short value;

    if (!gtz89_provider_camera_enabled(ctx) || !out_fov_deg_x100) return 0;

    if (ctx->provider.read_base_fov) {
        value = 0;
        if (ctx->provider.read_base_fov(ctx->provider.user, &value)) {
            *out_fov_deg_x100 = (short)GTZ89_CLAMP(value, 100, 17800);
            return 1;
        }
    }

    if (ctx->provider.camera) {
        value = ctx->provider.camera->base_fov_deg_x100;
        if (value >= 100 && value <= 17800) {
            *out_fov_deg_x100 = value;
            return 1;
        }
    }

    return 0;
}

static void gtz89_provider_write_outputs(gtz89_ctx *ctx)
{
    if (!gtz89_provider_camera_enabled(ctx)) return;

    if (ctx->provider.camera) {
        ctx->provider.camera->base_fov_deg_x100 = ctx->base_fov_deg_x100;
        ctx->provider.camera->current_fov_deg_x100 =
            ctx->current_fov_deg_x100;
    }

    if (ctx->provider.write_fov) {
        ctx->provider.write_fov(ctx->provider.user,
                                ctx->base_fov_deg_x100,
                                ctx->current_fov_deg_x100);
    }

    if (ctx->provider.write_sensitivity) {
        ctx->provider.write_sensitivity(ctx->provider.user,
                                        ctx->sensitivity_pct);
    }
}

static short gtz89_solve_fov_for_ctx(gtz89_ctx *ctx,
                                     short base_fov_deg_x100,
                                     short zoom_x100)
{
    short value;

    if (gtz89_provider_math_enabled(ctx) && ctx->provider.solve_fov) {
        value = 0;
        if (ctx->provider.solve_fov(ctx->provider.user,
                                    base_fov_deg_x100,
                                    zoom_x100,
                                    &value)) {
            return (short)GTZ89_CLAMP(value, 1, 17800);
        }
    }

    return gtz89_solve_fov_deg_x100(base_fov_deg_x100, zoom_x100);
}

static short gtz89_lerp_for_ctx(gtz89_ctx *ctx,
                                short a,
                                short b,
                                short t_x1000)
{
    short value;

    if (gtz89_provider_math_enabled(ctx) && ctx->provider.lerp_short) {
        value = 0;
        if (ctx->provider.lerp_short(ctx->provider.user,
                                     a, b, t_x1000, &value)) {
            return value;
        }
    }

    return gtz89_lerp_short_internal(a, b, t_x1000);
}

void gtz89_set_provider(gtz89_ctx *ctx, const gtz89_provider *provider)
{
    short external_base_fov;

    if (!ctx) return;
    if (!provider || provider->mode == GTZ89_PROVIDER_INTERNAL) {
        gtz89_clear_provider(ctx);
        return;
    }

    ctx->provider = *provider;
    ctx->provider.mode = (unsigned short)(provider->mode &
        GTZ89_PROVIDER_CAMERA_AND_MATH);
    ctx->provider_enabled = (short)(ctx->provider.mode !=
                                    GTZ89_PROVIDER_INTERNAL);

    external_base_fov = ctx->base_fov_deg_x100;
    if (gtz89_provider_read_base_fov(ctx, &external_base_fov)) {
        ctx->base_fov_deg_x100 = external_base_fov;
    }

    ctx->target_fov_deg_x100 = gtz89_solve_fov_for_ctx(
        ctx, ctx->base_fov_deg_x100, ctx->profile.zoom_x100);
    ctx->current_fov_deg_x100 = gtz89_lerp_for_ctx(
        ctx, ctx->base_fov_deg_x100,
        ctx->target_fov_deg_x100, ctx->blend_x1000);
    ctx->sensitivity_pct = gtz89_lerp_for_ctx(
        ctx, 100, ctx->profile.scoped_sens_pct, ctx->blend_x1000);
    gtz89_provider_write_outputs(ctx);
}

void gtz89_clear_provider(gtz89_ctx *ctx)
{
    if (!ctx) return;
    gtz89_provider_init(&ctx->provider);
    ctx->provider_enabled = 0;
    ctx->target_fov_deg_x100 = gtz89_solve_fov_deg_x100(
        ctx->base_fov_deg_x100, ctx->profile.zoom_x100);
}

unsigned short gtz89_get_provider_mode(const gtz89_ctx *ctx)
{
    if (!ctx || !ctx->provider_enabled) return GTZ89_PROVIDER_INTERNAL;
    return ctx->provider.mode;
}

void gtz89_init(gtz89_ctx *ctx,
                const gtz89_profile *profile,
                short base_fov_deg_x100)
{
    if (!ctx) return;
    if (profile) ctx->profile = *profile;
    else ctx->profile = gtz89_profile_sniper8x();
    ctx->active = 0;
    ctx->blend_x1000 = 0;
    ctx->base_fov_deg_x100 = (short)GTZ89_CLAMP(
        base_fov_deg_x100, 100, 17800);
    ctx->target_fov_deg_x100 = gtz89_solve_fov_deg_x100(
        ctx->base_fov_deg_x100, ctx->profile.zoom_x100);
    ctx->current_fov_deg_x100 = ctx->base_fov_deg_x100;
    ctx->sensitivity_pct = 100;
    ctx->just_entered = 0;
    ctx->just_exited = 0;
    gtz89_provider_init(&ctx->provider);
    ctx->provider_enabled = 0;
}

void gtz89_set_profile(gtz89_ctx *ctx, const gtz89_profile *profile)
{
    if (!ctx || !profile) return;
    ctx->profile = *profile;
    ctx->target_fov_deg_x100 = gtz89_solve_fov_for_ctx(
        ctx, ctx->base_fov_deg_x100, ctx->profile.zoom_x100);
}

void gtz89_set_base_fov(gtz89_ctx *ctx, short base_fov_deg_x100)
{
    if (!ctx) return;
    ctx->base_fov_deg_x100 = (short)GTZ89_CLAMP(
        base_fov_deg_x100, 100, 17800);
    ctx->target_fov_deg_x100 = gtz89_solve_fov_for_ctx(
        ctx, ctx->base_fov_deg_x100, ctx->profile.zoom_x100);
    gtz89_provider_write_outputs(ctx);
}

void gtz89_begin(gtz89_ctx *ctx)
{
    if (!ctx) return;
    if (!ctx->active) ctx->just_entered = 1;
    ctx->active = 1;
}

void gtz89_end(gtz89_ctx *ctx)
{
    if (!ctx) return;
    if (ctx->active) ctx->just_exited = 1;
    ctx->active = 0;
}

short gtz89_update(gtz89_ctx *ctx, short dt_frames)
{
    short frames;
    short target_blend;
    short step;
    short external_base_fov;

    if (!ctx) return 0;
    if (dt_frames < 1) dt_frames = 1;

    external_base_fov = ctx->base_fov_deg_x100;
    if (gtz89_provider_read_base_fov(ctx, &external_base_fov) &&
        external_base_fov != ctx->base_fov_deg_x100) {
        ctx->base_fov_deg_x100 = external_base_fov;
        ctx->target_fov_deg_x100 = gtz89_solve_fov_for_ctx(
            ctx, ctx->base_fov_deg_x100, ctx->profile.zoom_x100);
    }

    target_blend = ctx->active ? 1000 : 0;
    frames = ctx->active ? ctx->profile.enter_frames : ctx->profile.exit_frames;
    if (frames < 1) frames = 1;
    step = (short)((1000L * (long)dt_frames) / (long)frames);
    if (step < 1) step = 1;

    if (ctx->blend_x1000 < target_blend) {
        ctx->blend_x1000 = (short)GTZ89_CLAMP(
            ctx->blend_x1000 + step, 0, 1000);
    } else if (ctx->blend_x1000 > target_blend) {
        ctx->blend_x1000 = (short)GTZ89_CLAMP(
            ctx->blend_x1000 - step, 0, 1000);
    }

    ctx->current_fov_deg_x100 = gtz89_lerp_for_ctx(
        ctx, ctx->base_fov_deg_x100,
        ctx->target_fov_deg_x100, ctx->blend_x1000);
    ctx->sensitivity_pct = gtz89_lerp_for_ctx(
        ctx, 100, ctx->profile.scoped_sens_pct, ctx->blend_x1000);

    if (ctx->blend_x1000 == 1000) ctx->just_entered = 0;
    if (ctx->blend_x1000 == 0) ctx->just_exited = 0;

    gtz89_provider_write_outputs(ctx);
    return ctx->current_fov_deg_x100;
}

static gtz89_profile gtz89_make_profile(short zoom, short enter_frames,
                                        short exit_frames, short sens)
{
    gtz89_profile p;
    p.zoom_x100 = zoom;
    p.enter_frames = enter_frames;
    p.exit_frames = exit_frames;
    p.scoped_sens_pct = sens;
    return p;
}

gtz89_profile gtz89_profile_red_dot(void)
{
    return gtz89_make_profile(125, 6, 5, 80);
}

gtz89_profile gtz89_profile_acog4x(void)
{
    return gtz89_make_profile(400, 8, 6, 45);
}

gtz89_profile gtz89_profile_sniper8x(void)
{
    return gtz89_make_profile(800, 10, 8, 25);
}

gtz89_profile gtz89_profile_sniper12x(void)
{
    return gtz89_make_profile(1200, 12, 8, 18);
}


int gtz89_profile_from_recipe(const gri89_doc *doc, gtz89_profile *out_profile)
{
    gtz89_profile p;
    if (!doc || !out_profile) return 0;
    p.zoom_x100 = (short)gri89_get_long(doc, "zoom", "zoom_x100", 800);
    p.enter_frames = (short)gri89_get_long(doc, "zoom", "enter_frames", 10);
    p.exit_frames = (short)gri89_get_long(doc, "zoom", "exit_frames", 8);
    p.scoped_sens_pct = (short)gri89_get_long(doc, "zoom", "sensitivity_pct", 25);
    *out_profile = p;
    return 1;
}

int gtz89_profile_load_ini(const char *path, gtz89_profile *out_profile)
{
    gri89_doc doc;
    if (!path || !out_profile) return 0;
    gri89_init(&doc);
    if (!gri89_load(&doc, path)) return 0;
    return gtz89_profile_from_recipe(&doc, out_profile);
}
