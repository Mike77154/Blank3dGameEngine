#include "../gtelescopiczoom89/include/gtelescopiczoom89.h"

static short solve_calls;
static short lerp_calls;
static short sink_fov;
static short sink_sens;

static short test_solve(void *user, short base, short zoom, short *out)
{
    (void)user;
    (void)base;
    (void)zoom;
    ++solve_calls;
    *out = 1500;
    return 1;
}

static short test_lerp(void *user, short a, short b,
                       short t_x1000, short *out)
{
    (void)user;
    (void)a;
    (void)t_x1000;
    ++lerp_calls;
    *out = b;
    return 1;
}

static void test_write_fov(void *user, short base, short current)
{
    (void)user;
    (void)base;
    sink_fov = current;
}

static void test_write_sens(void *user, short sensitivity)
{
    (void)user;
    sink_sens = sensitivity;
}

static int test_internal(void)
{
    gtz89_ctx ctx;
    gtz89_profile profile;
    short fov;

    profile = gtz89_profile_sniper8x();
    gtz89_init(&ctx, &profile, 9000);
    if (gtz89_get_provider_mode(&ctx) != GTZ89_PROVIDER_INTERNAL) return 1;
    gtz89_begin(&ctx);
    fov = gtz89_update(&ctx, 1);
    if (fov >= 9000) return 2;
    return 0;
}

static int test_camera(void)
{
    gtz89_ctx ctx;
    gtz89_profile profile;
    gtz89_provider provider;
    g89_camera camera;

    camera.pos.x = G89_FX_FROM_INT(17);
    camera.pos.y = 0;
    camera.pos.z = 0;
    camera.forward.x = 0;
    camera.forward.y = 0;
    camera.forward.z = G89_FX_ONE;
    camera.right.x = G89_FX_ONE;
    camera.right.y = 0;
    camera.right.z = 0;
    camera.up.x = 0;
    camera.up.y = G89_FX_ONE;
    camera.up.z = 0;
    camera.base_fov_deg_x100 = 8000;
    camera.current_fov_deg_x100 = 8000;

    profile = gtz89_profile_acog4x();
    gtz89_init(&ctx, &profile, 9000);
    gtz89_provider_init(&provider);
    provider.mode = GTZ89_PROVIDER_CAMERA;
    provider.camera = &camera;
    provider.write_fov = test_write_fov;
    provider.write_sensitivity = test_write_sens;
    gtz89_set_provider(&ctx, &provider);
    gtz89_begin(&ctx);
    (void)gtz89_update(&ctx, 1);

    if (ctx.base_fov_deg_x100 != 8000) return 1;
    if (camera.current_fov_deg_x100 != ctx.current_fov_deg_x100) return 2;
    if (camera.pos.x != G89_FX_FROM_INT(17)) return 3;
    if (sink_fov != ctx.current_fov_deg_x100) return 4;
    if (sink_sens != ctx.sensitivity_pct) return 5;
    return 0;
}

static int test_math_and_both(void)
{
    gtz89_ctx ctx;
    gtz89_profile profile;
    gtz89_provider provider;
    g89_camera camera;

    camera.pos.x = G89_FX_FROM_INT(33);
    camera.pos.y = 0;
    camera.pos.z = 0;
    camera.forward.x = 0;
    camera.forward.y = 0;
    camera.forward.z = G89_FX_ONE;
    camera.right.x = G89_FX_ONE;
    camera.right.y = 0;
    camera.right.z = 0;
    camera.up.x = 0;
    camera.up.y = G89_FX_ONE;
    camera.up.z = 0;
    camera.base_fov_deg_x100 = 7000;
    camera.current_fov_deg_x100 = 7000;

    solve_calls = 0;
    lerp_calls = 0;
    profile = gtz89_profile_sniper12x();
    gtz89_init(&ctx, &profile, 9000);
    gtz89_provider_init(&provider);
    provider.mode = GTZ89_PROVIDER_CAMERA_AND_MATH;
    provider.camera = &camera;
    provider.solve_fov = test_solve;
    provider.lerp_short = test_lerp;
    gtz89_set_provider(&ctx, &provider);
    gtz89_begin(&ctx);
    (void)gtz89_update(&ctx, 1);

    if (gtz89_get_provider_mode(&ctx) !=
        GTZ89_PROVIDER_CAMERA_AND_MATH) return 1;
    if (ctx.target_fov_deg_x100 != 1500) return 2;
    if (ctx.current_fov_deg_x100 != 1500) return 3;
    if (camera.current_fov_deg_x100 != 1500) return 4;
    if (camera.pos.x != G89_FX_FROM_INT(33)) return 5;
    if (solve_calls < 1 || lerp_calls < 1) return 6;

    gtz89_clear_provider(&ctx);
    if (gtz89_get_provider_mode(&ctx) != GTZ89_PROVIDER_INTERNAL) return 7;
    return 0;
}

int main(void)
{
    int rc;
    rc = test_internal();
    if (rc) return 10 + rc;
    rc = test_camera();
    if (rc) return 20 + rc;
    rc = test_math_and_both();
    if (rc) return 30 + rc;
    return 0;
}
