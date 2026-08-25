#include "../include/gscopebundle89.h"
#include <string.h>

static int gscb89_external_missing(gscb89_ctx *ctx, short mode)
{
    if (mode == GPR89_MODE_EXTERNAL) {
        if (ctx) ctx->last_error = GSCB89_ERR_PROVIDER_REQUIRED;
        return 1;
    }
    return 0;
}

static int gscb89_provider_result(gscb89_ctx *ctx, int result, short mode)
{
    if (result > 0) return 1;
    if (result < 0) {
        if (ctx) ctx->last_error = GSCB89_ERR_PROVIDER;
        return -1;
    }
    if (mode == GPR89_MODE_EXTERNAL) {
        if (ctx) ctx->last_error = GSCB89_ERR_PROVIDER_REQUIRED;
        return -1;
    }
    return 0;
}

void gscb89_init(gscb89_ctx *ctx)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    gpr89_hub_init(&ctx->hub);
}

int gscb89_register_provider(gscb89_ctx *ctx,
                             const gpr89_provider *provider)
{
    if (!ctx) return 0;
    return gpr89_hub_register(&ctx->hub, provider);
}

void gscb89_set_provider_route(gscb89_ctx *ctx,
                                short domain,
                                short mode,
                                const char *provider_name)
{
    if (!ctx) return;
    gpr89_hub_set_route(&ctx->hub, domain, mode, provider_name);
}

int gscb89_apply_provider_recipe(gscb89_ctx *ctx, const gri89_doc *doc)
{
    if (!ctx || !doc) return 0;
    return gpr89_hub_apply_recipe(&ctx->hub, doc);
}

int gscb89_load_recipe(gscb89_ctx *ctx,
                       const char *path,
                       gri89_doc *out_doc)
{
    const gpr89_provider *provider;
    short mode;
    int result;
    if (!ctx || !path || !out_doc) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_RECIPE, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_RECIPE) && provider->load_recipe) {
        result = gscb89_provider_result(ctx,
            provider->load_recipe(provider->user, path, out_doc), mode);
        if (result > 0) {
            gpr89_hub_apply_recipe(&ctx->hub, out_doc);
            return 1;
        }
        if (result < 0) return 0;
    } else if (provider == 0 && gscb89_external_missing(ctx, mode)) {
        return 0;
    }
    gri89_init(out_doc);
    if (!gri89_load(out_doc, path)) {
        ctx->last_error = GSCB89_ERR_RECIPE;
        return 0;
    }
    gpr89_hub_apply_recipe(&ctx->hub, out_doc);
    return 1;
}

static void gscb89_paint_bridge(void *user, const gsp89_draw_cmd *cmd)
{
    gscb89_ctx *ctx;
    const gpr89_provider *provider;
    short mode;
    int result;
    ctx = (gscb89_ctx *)user;
    if (!ctx || !cmd) return;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_PAINT, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_PAINT) && provider->emit_draw_cmd) {
        result = provider->emit_draw_cmd(provider->user, cmd);
        if (result > 0) return;
        if (result < 0 || mode == GPR89_MODE_EXTERNAL) {
            ctx->last_error = result < 0 ? GSCB89_ERR_PROVIDER : GSCB89_ERR_PROVIDER_REQUIRED;
            return;
        }
    } else if (mode == GPR89_MODE_EXTERNAL) {
        ctx->last_error = GSCB89_ERR_PROVIDER_REQUIRED;
        return;
    }
    if (ctx->paint_sink) ctx->paint_sink(ctx->paint_sink_user, cmd);
}

void gscb89_painter_init(gscb89_ctx *ctx,
                          gsp89_painter *painter,
                          short screen_w,
                          short screen_h,
                          gsp89_emit_cb final_sink,
                          void *final_sink_user)
{
    if (!ctx || !painter) return;
    ctx->paint_sink = final_sink;
    ctx->paint_sink_user = final_sink_user;
    gsp89_painter_init(painter, screen_w, screen_h, gscb89_paint_bridge, ctx);
}

int gscb89_emit_primitive(gscb89_ctx *ctx,
                          gsp89_painter *painter,
                          const gsv89_shape *shape,
                          const gsv89_palette *palette,
                          short global_alpha)
{
    const gpr89_provider *provider;
    short mode;
    int result;
    if (!ctx || !painter || !shape) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_PRIMITIVE, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_PRIMITIVE) && provider->emit_primitive) {
        result = gscb89_provider_result(ctx,
            provider->emit_primitive(provider->user, painter, shape,
                                     palette, global_alpha), mode);
        if (result > 0) return 1;
        if (result < 0) return 0;
    } else if (provider == 0 && gscb89_external_missing(ctx, mode)) {
        return 0;
    }
    gsv89_emit_shape(painter, shape, palette, global_alpha);
    return 1;
}

int gscb89_emit_vector(gscb89_ctx *ctx,
                       gsp89_painter *painter,
                       const gsv89_shape *shapes,
                       short shape_count,
                       const gsv89_palette *palette,
                       short global_alpha)
{
    const gpr89_provider *provider;
    short mode;
    short i;
    int result;
    if (!ctx || !painter || !shapes || shape_count < 1) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_VECTOR, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_VECTOR) && provider->emit_vector) {
        result = gscb89_provider_result(ctx,
            provider->emit_vector(provider->user, painter, shapes, shape_count,
                                  palette, global_alpha), mode);
        if (result > 0) return 1;
        if (result < 0) return 0;
    } else if (provider == 0 && gscb89_external_missing(ctx, mode)) {
        return 0;
    }
    for (i = 0; i < shape_count; ++i) {
        if (!gscb89_emit_primitive(ctx, painter, &shapes[i], palette, global_alpha))
            return 0;
    }
    return 1;
}

int gscb89_emit_raster_layer(gscb89_ctx *ctx,
                              gsp89_painter *painter,
                              const gsr89_layer *layer,
                              short global_alpha)
{
    const gpr89_provider *provider;
    short mode;
    int result;
    if (!ctx || !painter || !layer) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_RASTER, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_RASTER) && provider->emit_raster) {
        result = gscb89_provider_result(ctx,
            provider->emit_raster(provider->user, painter, layer, global_alpha), mode);
        if (result > 0) return 1;
        if (result < 0) return 0;
    } else if (provider == 0 && gscb89_external_missing(ctx, mode)) {
        return 0;
    }
    gsr89_emit_layer(painter, layer, global_alpha);
    return 1;
}

int gscb89_emit_raster_stack(gscb89_ctx *ctx,
                              gsp89_painter *painter,
                              const gsr89_stack *stack,
                              short global_alpha)
{
    short i;
    if (!ctx || !painter || !stack || !stack->layers) return 0;
    if (stack->clip_to_scope)
        gsp89_clip_circle_begin(painter, 0, 0,
            stack->clip_radius <= 0 ? GSP89_FX_ONE : stack->clip_radius);
    for (i = 0; i < stack->layer_count; ++i) {
        if (!gscb89_emit_raster_layer(ctx, painter, &stack->layers[i], global_alpha)) {
            if (stack->clip_to_scope) gsp89_clip_end(painter);
            return 0;
        }
    }
    if (stack->clip_to_scope) gsp89_clip_end(painter);
    return 1;
}

int gscb89_emit_bar(gscb89_ctx *ctx,
                    gsp89_painter *painter,
                    const gsb89_bar *bar,
                    const gsb89_channel *channel,
                    short global_alpha)
{
    const gpr89_provider *provider;
    short mode;
    int result;
    if (!ctx || !painter || !bar || !channel) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_BARS, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_BARS) && provider->emit_bar) {
        result = gscb89_provider_result(ctx,
            provider->emit_bar(provider->user, painter, bar, channel, global_alpha), mode);
        if (result > 0) return 1;
        if (result < 0) return 0;
    } else if (provider == 0 && gscb89_external_missing(ctx, mode)) {
        return 0;
    }
    gsb89_emit_bar(painter, bar, channel, global_alpha);
    return 1;
}

int gscb89_emit_bar_layout(gscb89_ctx *ctx,
                           gsp89_painter *painter,
                           const gsb89_layout *layout,
                           const gsb89_channel *channels,
                           short channel_count,
                           short global_alpha)
{
    short i;
    gsb89_channel channel;
    if (!ctx || !painter || !layout || !layout->bars || !channels) return 0;
    for (i = 0; i < layout->bar_count; ++i) {
        if (gsb89_find_channel(channels, channel_count,
                               layout->bars[i].channel_id, &channel)) {
            if (!gscb89_emit_bar(ctx, painter, &layout->bars[i], &channel, global_alpha))
                return 0;
        }
    }
    return 1;
}

const gsvp89_preset *gscb89_find_preset(gscb89_ctx *ctx, const char *name)
{
    const gpr89_provider *provider;
    const gsvp89_preset *preset;
    short mode;
    if (!ctx || !name) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_PRESET, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_PRESET) && provider->find_preset) {
        preset = provider->find_preset(provider->user, name);
        if (preset) return preset;
        if (mode == GPR89_MODE_EXTERNAL) {
            ctx->last_error = GSCB89_ERR_PROVIDER_REQUIRED;
            return 0;
        }
    } else if (provider == 0 && gscb89_external_missing(ctx, mode)) {
        return 0;
    }
    return gsvp89_find(name);
}

const gsvp89_preset *gscb89_preset_from_recipe(gscb89_ctx *ctx,
                                                    const gri89_doc *doc,
                                                    const char *section_name)
{
    const char *name;
    if (!ctx || !doc || !section_name) return 0;
    name = gri89_get(doc, section_name, "use", 0);
    if (!name || !name[0]) return 0;
    return gscb89_find_preset(ctx, name);
}

int gscb89_emit_preset(gscb89_ctx *ctx,
                       gsp89_painter *painter,
                       const gsvp89_preset *preset,
                       const gsv89_palette *palette_override,
                       short global_alpha)
{
    gsv89_palette palette;
    const gsv89_palette *use_palette;
    if (!ctx || !painter || !preset) return 0;
    use_palette = palette_override;
    if (!use_palette) {
        gsvp89_default_palette(preset, &palette);
        use_palette = &palette;
    }
    return gscb89_emit_vector(ctx, painter, preset->shapes,
                              preset->shape_count, use_palette, global_alpha);
}

static void gscb89_hud_bridge(void *user, const gsh89_cmd *cmd)
{
    gscb89_ctx *ctx;
    const gpr89_provider *provider;
    short mode;
    int result;
    ctx = (gscb89_ctx *)user;
    if (!ctx || !cmd) return;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_HUD, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_HUD) && provider->emit_hud_cmd) {
        result = provider->emit_hud_cmd(provider->user, cmd);
        if (result > 0) return;
        if (result < 0 || mode == GPR89_MODE_EXTERNAL) {
            ctx->last_error = result < 0 ? GSCB89_ERR_PROVIDER : GSCB89_ERR_PROVIDER_REQUIRED;
            return;
        }
    } else if (mode == GPR89_MODE_EXTERNAL) {
        ctx->last_error = GSCB89_ERR_PROVIDER_REQUIRED;
        return;
    }
    if (ctx->hud_sink) ctx->hud_sink(ctx->hud_sink_user, cmd);
}

void gscb89_hud_init(gscb89_ctx *ctx,
                      gsh89_ctx *hud,
                      const gsh89_profile *profile,
                      short screen_w,
                      short screen_h,
                      gsh89_emit_cb final_sink,
                      void *final_sink_user)
{
    if (!ctx || !hud) return;
    ctx->hud_sink = final_sink;
    ctx->hud_sink_user = final_sink_user;
    gsh89_init(hud, profile, screen_w, screen_h, gscb89_hud_bridge, ctx);
}

int gscb89_fill_telemetry(gscb89_ctx *ctx,
                          gsh89_telemetry *inout_telemetry)
{
    const gpr89_provider *provider;
    short mode;
    int result;
    if (!ctx || !inout_telemetry) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_TELEMETRY, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_TELEMETRY) && provider->fill_telemetry) {
        result = gscb89_provider_result(ctx,
            provider->fill_telemetry(provider->user, inout_telemetry), mode);
        return result > 0 ? 1 : 0;
    }
    if (provider == 0 && gscb89_external_missing(ctx, mode)) return 0;
    /* There is no generic internal telemetry source; caller-owned values remain valid. */
    return 1;
}

int gscb89_resolve_asset(gscb89_ctx *ctx,
                         const char *asset_name,
                         short *out_asset_id)
{
    const gpr89_provider *provider;
    short mode;
    int result;
    if (!ctx || !asset_name || !out_asset_id) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_ASSET, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_ASSET) && provider->resolve_asset) {
        result = gscb89_provider_result(ctx,
            provider->resolve_asset(provider->user, asset_name, out_asset_id), mode);
        if (result > 0) return 1;
        if (result < 0) return 0;
    } else if (provider == 0 && gscb89_external_missing(ctx, mode)) {
        return 0;
    }
    return 0;
}

int gscb89_bind_zoom_provider(gscb89_ctx *ctx, gtz89_ctx *zoom)
{
    const gpr89_provider *provider;
    gtz89_provider zp;
    short mode;
    if (!ctx || !zoom) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_ZOOM, &mode);
    if (!provider) {
        if (mode == GPR89_MODE_EXTERNAL) {
            ctx->last_error = GSCB89_ERR_PROVIDER_REQUIRED;
            return 0;
        }
        gtz89_clear_provider(zoom);
        return 1;
    }
    if (!(provider->capabilities & GPR89_CAP_ZOOM)) {
        if (mode == GPR89_MODE_EXTERNAL) {
            ctx->last_error = GSCB89_ERR_PROVIDER_REQUIRED;
            return 0;
        }
        gtz89_clear_provider(zoom);
        return 1;
    }
    gtz89_provider_init(&zp);
    zp.user = provider->user;
    zp.camera = provider->zoom_camera;
    zp.read_base_fov = provider->zoom_read_base_fov;
    zp.write_fov = provider->zoom_write_fov;
    zp.write_sensitivity = provider->zoom_write_sensitivity;
    zp.solve_fov = provider->zoom_solve_fov;
    zp.lerp_short = provider->zoom_lerp_short;
    zp.mode = GTZ89_PROVIDER_INTERNAL;
    if (zp.camera || zp.read_base_fov || zp.write_fov || zp.write_sensitivity)
        zp.mode = (unsigned short)(zp.mode | GTZ89_PROVIDER_CAMERA);
    if (zp.solve_fov || zp.lerp_short)
        zp.mode = (unsigned short)(zp.mode | GTZ89_PROVIDER_MATH);
    if (zp.mode == GTZ89_PROVIDER_INTERNAL) {
        if (mode == GPR89_MODE_EXTERNAL) {
            ctx->last_error = GSCB89_ERR_PROVIDER_REQUIRED;
            return 0;
        }
        gtz89_clear_provider(zoom);
        return 1;
    }
    gtz89_set_provider(zoom, &zp);
    return 1;
}

int gscb89_emit_animated_preset(gscb89_ctx *ctx,
                                gsp89_painter *painter,
                                const gsvp89_preset *preset,
                                const gsv89_palette *palette_override,
                                const gsa89_pose *pose,
                                gsv89_shape *scratch_shapes,
                                short scratch_capacity,
                                short global_alpha)
{
    const gpr89_provider *provider;
    gsv89_palette palette;
    const gsv89_palette *use_palette;
    short mode;
    int result;
    short count;
    short animated_alpha;
    if (!ctx || !painter || !preset || !pose || !scratch_shapes) return 0;
    if (scratch_capacity < preset->shape_count) return 0;
    provider = gpr89_hub_provider_for(&ctx->hub, GPR89_DOMAIN_ANIMATION, &mode);
    if (provider && (provider->capabilities & GPR89_CAP_ANIMATION) && provider->animate_shapes) {
        result = gscb89_provider_result(ctx,
            provider->animate_shapes(provider->user, pose, preset->shapes,
                                     preset->shape_count, scratch_shapes,
                                     scratch_capacity), mode);
        if (result < 0) return 0;
        if (result == 0) {
            count = gsa89_apply_shapes(pose, preset->shapes, preset->shape_count,
                                       scratch_shapes, scratch_capacity);
        } else {
            count = preset->shape_count;
        }
    } else {
        if (provider == 0 && gscb89_external_missing(ctx, mode)) return 0;
        count = gsa89_apply_shapes(pose, preset->shapes, preset->shape_count,
                                   scratch_shapes, scratch_capacity);
    }
    if (count != preset->shape_count) return 0;
    use_palette = palette_override;
    if (!use_palette) {
        gsvp89_default_palette(preset, &palette);
        use_palette = &palette;
    }
    animated_alpha = gsa89_apply_alpha(pose, global_alpha, GSA89_TARGET_ROOT);
    return gscb89_emit_vector(ctx, painter, scratch_shapes, count,
                              use_palette, animated_alpha);
}
