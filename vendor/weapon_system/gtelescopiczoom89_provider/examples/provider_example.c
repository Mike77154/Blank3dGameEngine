#include "../gtelescopiczoom89/include/gtelescopiczoom89.h"

/* Externally owned by the engine. Transform vectors are never overwritten. */
static g89_camera engine_camera;
static short engine_scoped_sensitivity;

static void engine_write_sensitivity(void *user, short sensitivity_pct)
{
    short *dst;
    dst = (short *)user;
    *dst = sensitivity_pct;
}

int main(void)
{
    gtz89_ctx zoom;
    gtz89_profile profile;
    gtz89_provider provider;
    short i;

    engine_camera.base_fov_deg_x100 = 9000;
    engine_camera.current_fov_deg_x100 = 9000;
    engine_camera.pos.x = G89_FX_FROM_INT(10);

    profile = gtz89_profile_sniper8x();
    gtz89_init(&zoom, &profile, 9000);

    gtz89_provider_init(&provider);
    provider.mode = GTZ89_PROVIDER_CAMERA;
    provider.camera = &engine_camera;
    provider.user = &engine_scoped_sensitivity;
    provider.write_sensitivity = engine_write_sensitivity;
    gtz89_set_provider(&zoom, &provider);

    gtz89_begin(&zoom);
    for (i = 0; i < 12; ++i) {
        (void)gtz89_update(&zoom, 1);
    }

    /* Camera transform is preserved; only FOV fields were synchronized. */
    if (engine_camera.pos.x != G89_FX_FROM_INT(10)) return 1;
    if (engine_camera.current_fov_deg_x100 >= 9000) return 2;
    if (engine_scoped_sensitivity != profile.scoped_sens_pct) return 3;
    return 0;
}
