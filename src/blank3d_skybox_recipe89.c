#include "blank3d_skybox_recipe89.h"

#include <stdio.h>
#include <string.h>

static void b3d_sbr_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void b3d_sbr_status(Blank3DSkyboxRecipe89 *ctx, const char *text)
{
    if (!ctx) return;
    b3d_sbr_copy(ctx->status, (unsigned int)sizeof(ctx->status), text);
}

static int b3d_sbr_file_load(void *user, const char *path,
                             char *out_text, unsigned int capacity,
                             unsigned int *out_size)
{
    FILE *fp;
    long size;
    unsigned int got;
    (void)user;
    if (!path || !out_text || capacity == 0U || !out_size) return 0;
    fp = fopen(path, "rb");
    if (!fp) return 0;
    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return 0;
    }
    size = ftell(fp);
    if (size < 0L || (unsigned long)size + 1UL > (unsigned long)capacity) {
        fclose(fp);
        return 0;
    }
    rewind(fp);
    got = (unsigned int)fread(out_text, 1U, (size_t)size, fp);
    fclose(fp);
    if (got != (unsigned int)size) return 0;
    out_text[got] = '\0';
    *out_size = got;
    return 1;
}

static void b3d_sbr_color(Gskybox89_Color *dst, const SBR89Color *src)
{
    if (!dst || !src) return;
    dst->r = src->r;
    dst->g = src->g;
    dst->b = src->b;
    dst->a = src->a;
}

static int b3d_sbr_apply(Blank3DSkyboxRecipe89 *ctx,
                         const SBR89Recipe *recipe)
{
    Blank3DSkybox89 *sky;
    int i;
    if (!ctx || !ctx->skybox || !recipe) return 0;
    sky = ctx->skybox;

    blank3d_skybox89_set_enabled(sky, recipe->enabled);
    blank3d_skybox89_set_layers(sky,
                                recipe->screen_enabled,
                                recipe->cube_enabled,
                                recipe->dome_enabled);
    blank3d_skybox89_set_radius(sky, (g3d_fix)(recipe->radius_q16 / 16L));
    blank3d_skybox89_set_uv_inset_pixels(sky, recipe->uv_inset_pixels);
    (void)blank3d_skybox89_set_screen_request(sky, recipe->screen_image);
    (void)blank3d_skybox89_set_dome_request(sky, recipe->dome_image);

    for (i = 0; i < B3D_SKYBOX89_FACE_COUNT; ++i) {
        (void)blank3d_skybox89_set_face_request(sky, i,
                                                recipe->face_image[i]);
        (void)blank3d_skybox89_set_face_uv_q16(
            sky, i,
            recipe->face_uv[i].u0_q16,
            recipe->face_uv[i].v0_q16,
            recipe->face_uv[i].u1_q16,
            recipe->face_uv[i].v1_q16);
    }

    sky->core_config.screen_depth_func = recipe->screen_depth_func;
    sky->core_config.dome_segments = recipe->dome_segments;
    sky->core_config.dome_rings = recipe->dome_rings;
    sky->core_config.dome_blend_mode = recipe->dome_blend_mode;
    b3d_sbr_color(&sky->core_config.screen_top, &recipe->screen_top);
    b3d_sbr_color(&sky->core_config.screen_bottom, &recipe->screen_bottom);
    b3d_sbr_color(&sky->core_config.dome_top, &recipe->dome_top);
    b3d_sbr_color(&sky->core_config.dome_horizon, &recipe->dome_horizon);
    gskybox89_cube_set_uv_xform(&sky->core_config,
                                recipe->flip_u_mask,
                                recipe->flip_v_mask,
                                recipe->swap_uv_mask);
    sky->core.cfg = sky->core_config;
    (void)blank3d_skybox89_resolve_assets(sky);
    blank3d_skybox89_gl_attach(sky);
    return 1;
}

void blank3d_skybox_recipe89_init(Blank3DSkyboxRecipe89 *ctx,
                                  Blank3DSkybox89 *skybox)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->skybox = skybox;
    ctx->io.user = ctx;
    ctx->io.load_text = b3d_sbr_file_load;
    sbr89_workspace_init(&ctx->workspace);
    sbr89_recipe_defaults(&ctx->active);
    b3d_sbr_copy(ctx->catalog_path, SBR89_PATH_CAP,
                 "config/skybox/catalog.ini");
    b3d_sbr_status(ctx, "skybox recipes initialized");
}

void blank3d_skybox_recipe89_set_catalog(Blank3DSkyboxRecipe89 *ctx,
                                         const char *catalog_path)
{
    if (!ctx) return;
    b3d_sbr_copy(ctx->catalog_path, SBR89_PATH_CAP,
                 catalog_path && catalog_path[0]
                 ? catalog_path : "config/skybox/catalog.ini");
}

int blank3d_skybox_recipe89_apply_named(Blank3DSkyboxRecipe89 *ctx,
                                        const char *recipe_name)
{
    char message[192];
    if (!ctx || !recipe_name || !recipe_name[0]) return 0;
    if (!sbr89_load_named_recipe(&ctx->active, &ctx->workspace, &ctx->io,
                                 ctx->catalog_path, recipe_name)) {
        sprintf(message, "skybox recipe failed: %s line=%d",
                sbr89_last_error(&ctx->workspace),
                sbr89_last_error_line(&ctx->workspace));
        b3d_sbr_status(ctx, message);
        return 0;
    }
    if (!b3d_sbr_apply(ctx, &ctx->active)) {
        b3d_sbr_status(ctx, "skybox recipe apply failed");
        return 0;
    }
    b3d_sbr_copy(ctx->active_name, SBR89_NAME_CAP, recipe_name);
    b3d_sbr_copy(ctx->active_ref, SBR89_PATH_CAP, recipe_name);
    ctx->active_is_path = 0;
    sprintf(message, "skybox recipe active: %.63s", ctx->active_name);
    b3d_sbr_status(ctx, message);
    return 1;
}

int blank3d_skybox_recipe89_apply_path(Blank3DSkyboxRecipe89 *ctx,
                                       const char *recipe_path)
{
    char message[192];
    if (!ctx || !recipe_path || !recipe_path[0]) return 0;
    if (!sbr89_load_recipe(&ctx->active, &ctx->workspace, &ctx->io,
                           recipe_path)) {
        sprintf(message, "skybox recipe path failed: %s line=%d",
                sbr89_last_error(&ctx->workspace),
                sbr89_last_error_line(&ctx->workspace));
        b3d_sbr_status(ctx, message);
        return 0;
    }
    if (!b3d_sbr_apply(ctx, &ctx->active)) return 0;
    b3d_sbr_copy(ctx->active_name, SBR89_NAME_CAP, recipe_path);
    b3d_sbr_copy(ctx->active_ref, SBR89_PATH_CAP, recipe_path);
    ctx->active_is_path = 1;
    b3d_sbr_status(ctx, "skybox recipe path active");
    return 1;
}

int blank3d_skybox_recipe89_reload(Blank3DSkyboxRecipe89 *ctx)
{
    char ref[SBR89_PATH_CAP];
    int is_path;
    if (!ctx || !ctx->active_ref[0]) return 0;
    b3d_sbr_copy(ref, SBR89_PATH_CAP, ctx->active_ref);
    is_path = ctx->active_is_path;
    if (is_path) return blank3d_skybox_recipe89_apply_path(ctx, ref);
    return blank3d_skybox_recipe89_apply_named(ctx, ref);
}

void blank3d_skybox_recipe89_disable(Blank3DSkyboxRecipe89 *ctx)
{
    if (!ctx || !ctx->skybox) return;
    blank3d_skybox89_set_enabled(ctx->skybox, 0);
    b3d_sbr_status(ctx, "skybox disabled");
}

const char *blank3d_skybox_recipe89_status(const Blank3DSkyboxRecipe89 *ctx)
{
    return ctx ? ctx->status : "skybox recipe runtime unavailable";
}

const char *blank3d_skybox_recipe89_active_name(const Blank3DSkyboxRecipe89 *ctx)
{
    return ctx ? ctx->active_name : "";
}
