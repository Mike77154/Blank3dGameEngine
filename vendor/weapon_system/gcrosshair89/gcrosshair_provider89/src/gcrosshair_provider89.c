#include <string.h>
#include "gcrosshair_provider89.h"

typedef struct GC89P_PrimitiveMux {
    const GC89P_Runtime *runtime;
    int translate_x;
    int translate_y;
} GC89P_PrimitiveMux;

static GC89_Fixed gc89p_fx_mul(GC89_Fixed a, GC89_Fixed b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long ah;
    unsigned long al;
    unsigned long bh;
    unsigned long bl;
    unsigned long out;
    int negative;

    negative = 0;
    if (a < 0) {
        ua = (unsigned long)(-a);
        negative = !negative;
    } else {
        ua = (unsigned long)a;
    }
    if (b < 0) {
        ub = (unsigned long)(-b);
        negative = !negative;
    } else {
        ub = (unsigned long)b;
    }
    ah = ua >> 16;
    al = ua & 65535UL;
    bh = ub >> 16;
    bl = ub & 65535UL;
    out = (ah * bh << 16) + (ah * bl) + (al * bh) + ((al * bl) >> 16);
    if (negative) return -(GC89_Fixed)out;
    return (GC89_Fixed)out;
}

static int gc89p_fx_to_int_round(GC89_Fixed value)
{
    if (value < 0) {
        return -(int)(((-value) + GC89_FX_HALF) >> GC89_FX_SHIFT);
    }
    return (int)((value + GC89_FX_HALF) >> GC89_FX_SHIFT);
}

static int gc89p_scaled_px(GC89_Fixed value_fx,
                           GC89_Fixed scale_fx,
                           int minimum)
{
    int value;
    value = gc89p_fx_to_int_round(gc89p_fx_mul(value_fx, scale_fx));
    if (value < minimum) value = minimum;
    return value;
}

static void gc89p_zero_callbacks(GC89_DrawCallbacks *callbacks)
{
    if (!callbacks) return;
    callbacks->draw_line = 0;
    callbacks->draw_dot = 0;
    callbacks->draw_image = 0;
}

void gc89p_runtime_init(GC89P_Runtime *runtime)
{
    if (!runtime) return;
    memset(runtime, 0, sizeof(*runtime));
    runtime->allow_internal_vector_fallback = 1;
    runtime->allow_primitive_fallback = 1;
}

void gc89p_runtime_set_vector_provider(GC89P_Runtime *runtime,
                                       const GC89P_VectorProvider *provider,
                                       void *user)
{
    if (!runtime) return;
    if (provider) runtime->vector_provider = *provider;
    else memset(&runtime->vector_provider, 0, sizeof(runtime->vector_provider));
    runtime->vector_user = user;
}

void gc89p_runtime_set_primitive_provider(GC89P_Runtime *runtime,
                                          const GC89_DrawCallbacks *provider,
                                          void *user)
{
    if (!runtime) return;
    if (provider) runtime->primitive_provider = *provider;
    else gc89p_zero_callbacks(&runtime->primitive_provider);
    runtime->primitive_user = user;
}

void gc89p_runtime_set_primitive_fallback(GC89P_Runtime *runtime,
                                          const GC89_DrawCallbacks *provider,
                                          void *user)
{
    if (!runtime) return;
    if (provider) runtime->primitive_fallback = *provider;
    else gc89p_zero_callbacks(&runtime->primitive_fallback);
    runtime->primitive_fallback_user = user;
}

void gc89p_runtime_set_asset_provider(GC89P_Runtime *runtime,
                                      const GC89P_AssetProvider *provider,
                                      void *user)
{
    if (!runtime) return;
    if (provider) runtime->asset_provider = *provider;
    else memset(&runtime->asset_provider, 0, sizeof(runtime->asset_provider));
    runtime->asset_user = user;
}

void gc89p_runtime_set_hud_provider(GC89P_Runtime *runtime,
                                    const GC89P_HudProvider *provider,
                                    void *user)
{
    if (!runtime) return;
    if (provider) runtime->hud_provider = *provider;
    else memset(&runtime->hud_provider, 0, sizeof(runtime->hud_provider));
    runtime->hud_user = user;
}

void gc89p_runtime_enable_internal_vector_fallback(GC89P_Runtime *runtime,
                                                    int enabled)
{
    if (!runtime) return;
    runtime->allow_internal_vector_fallback = enabled ? 1 : 0;
}

void gc89p_runtime_enable_primitive_fallback(GC89P_Runtime *runtime,
                                             int enabled)
{
    if (!runtime) return;
    runtime->allow_primitive_fallback = enabled ? 1 : 0;
}

static void gc89p_mux_line(void *user,
                           int x0, int y0, int x1, int y1,
                           int thickness_px,
                           unsigned long color_rgba)
{
    GC89P_PrimitiveMux *mux;
    const GC89P_Runtime *runtime;
    mux = (GC89P_PrimitiveMux *)user;
    if (!mux || !mux->runtime) return;
    runtime = mux->runtime;
    if (runtime->primitive_provider.draw_line) {
        runtime->primitive_provider.draw_line(runtime->primitive_user,
                                              x0 + mux->translate_x,
                                              y0 + mux->translate_y,
                                              x1 + mux->translate_x,
                                              y1 + mux->translate_y,
                                              thickness_px, color_rgba);
        return;
    }
    if (runtime->allow_primitive_fallback &&
        runtime->primitive_fallback.draw_line) {
        runtime->primitive_fallback.draw_line(runtime->primitive_fallback_user,
                                              x0 + mux->translate_x,
                                              y0 + mux->translate_y,
                                              x1 + mux->translate_x,
                                              y1 + mux->translate_y,
                                              thickness_px, color_rgba);
    }
}

static void gc89p_mux_dot(void *user,
                          int center_x, int center_y,
                          int diameter_px,
                          unsigned long color_rgba)
{
    GC89P_PrimitiveMux *mux;
    const GC89P_Runtime *runtime;
    mux = (GC89P_PrimitiveMux *)user;
    if (!mux || !mux->runtime) return;
    runtime = mux->runtime;
    if (runtime->primitive_provider.draw_dot) {
        runtime->primitive_provider.draw_dot(runtime->primitive_user,
                                             center_x + mux->translate_x,
                                             center_y + mux->translate_y,
                                             diameter_px, color_rgba);
        return;
    }
    if (runtime->allow_primitive_fallback &&
        runtime->primitive_fallback.draw_dot) {
        runtime->primitive_fallback.draw_dot(runtime->primitive_fallback_user,
                                             center_x + mux->translate_x,
                                             center_y + mux->translate_y,
                                             diameter_px, color_rgba);
    }
}

static void gc89p_mux_image(void *user,
                            int image_id,
                            int x, int y,
                            int width, int height,
                            unsigned long tint_rgba)
{
    GC89P_PrimitiveMux *mux;
    const GC89P_Runtime *runtime;
    mux = (GC89P_PrimitiveMux *)user;
    if (!mux || !mux->runtime) return;
    runtime = mux->runtime;
    if (runtime->primitive_provider.draw_image) {
        runtime->primitive_provider.draw_image(runtime->primitive_user,
                                               image_id, x, y,
                                               width, height, tint_rgba);
        return;
    }
    if (runtime->allow_primitive_fallback &&
        runtime->primitive_fallback.draw_image) {
        runtime->primitive_fallback.draw_image(runtime->primitive_fallback_user,
                                               image_id, x, y,
                                               width, height, tint_rgba);
    }
}

static int gc89p_have_line(const GC89P_Runtime *runtime)
{
    if (!runtime) return 0;
    if (runtime->primitive_provider.draw_line) return 1;
    if (runtime->allow_primitive_fallback &&
        runtime->primitive_fallback.draw_line) return 1;
    return 0;
}

static int gc89p_have_dot(const GC89P_Runtime *runtime)
{
    if (!runtime) return 0;
    if (runtime->primitive_provider.draw_dot) return 1;
    if (runtime->allow_primitive_fallback &&
        runtime->primitive_fallback.draw_dot) return 1;
    return 0;
}

static int gc89p_have_image(const GC89P_Runtime *runtime)
{
    if (!runtime) return 0;
    if (runtime->primitive_provider.draw_image) return 1;
    if (runtime->allow_primitive_fallback &&
        runtime->primitive_fallback.draw_image) return 1;
    return 0;
}

static void gc89p_resolve_viewport(const GC89P_Runtime *runtime,
                                   int fallback_width,
                                   int fallback_height,
                                   int *out_width,
                                   int *out_height,
                                   int *out_hud_used)
{
    int width;
    int height;
    int used;
    width = fallback_width;
    height = fallback_height;
    used = 0;
    if (runtime && runtime->hud_provider.get_viewport) {
        int candidate_w;
        int candidate_h;
        candidate_w = width;
        candidate_h = height;
        if (runtime->hud_provider.get_viewport(runtime->hud_user,
                                               &candidate_w,
                                               &candidate_h) &&
            candidate_w > 0 && candidate_h > 0) {
            width = candidate_w;
            height = candidate_h;
            used = 1;
        }
    }
    *out_width = width;
    *out_height = height;
    if (out_hud_used) *out_hud_used = used;
}

static void gc89p_resolve_center(const GC89P_Runtime *runtime,
                                 const GC89_Core *core,
                                 const GC89_DrawSpec *spec,
                                 int width,
                                 int height,
                                 int *out_x,
                                 int *out_y,
                                 int *out_hud_used)
{
    int x;
    int y;
    int provider_x;
    int provider_y;
    x = width / 2;
    y = height / 2;
    if (runtime && runtime->hud_provider.get_anchor) {
        provider_x = x;
        provider_y = y;
        if (runtime->hud_provider.get_anchor(runtime->hud_user,
                                             width, height,
                                             &provider_x,
                                             &provider_y)) {
            x = provider_x;
            y = provider_y;
            if (out_hud_used) *out_hud_used = 1;
        }
    }
    x += gc89p_fx_to_int_round(gc89p_fx_mul(spec->center_offset_x_fx,
                                            core->scale_fx));
    y += gc89p_fx_to_int_round(gc89p_fx_mul(spec->center_offset_y_fx,
                                            core->scale_fx));
    *out_x = x;
    *out_y = y;
}

static int gc89p_draw_internal_vector(const GC89P_Runtime *runtime,
                                      const GC89_Core *core,
                                      int width,
                                      int height,
                                      int desired_center_x,
                                      int desired_center_y,
                                      const GC89_DrawSpec *spec)
{
    GC89_DrawSpec local_spec;
    GC89_DrawCallbacks callbacks;
    GC89P_PrimitiveMux mux;
    GC89_DrawResult result;
    int core_center_x;
    int core_center_y;

    if (!runtime->allow_internal_vector_fallback) return 0;
    if (!gc89p_have_line(runtime) && !gc89p_have_dot(runtime)) return 0;

    local_spec = *spec;
    local_spec.draw_mode = GC89_DRAW_VECTOR;
    core_center_x = width / 2 +
        gc89p_fx_to_int_round(gc89p_fx_mul(spec->center_offset_x_fx,
                                           core->scale_fx));
    core_center_y = height / 2 +
        gc89p_fx_to_int_round(gc89p_fx_mul(spec->center_offset_y_fx,
                                           core->scale_fx));

    callbacks.draw_line = gc89p_have_line(runtime) ? gc89p_mux_line : 0;
    callbacks.draw_dot = gc89p_have_dot(runtime) ? gc89p_mux_dot : 0;
    callbacks.draw_image = 0;
    mux.runtime = runtime;
    mux.translate_x = desired_center_x - core_center_x;
    mux.translate_y = desired_center_y - core_center_y;
    return gc89_core_draw(core, width, height, &local_spec,
                          &callbacks, &mux, &result);
}

int gc89p_draw_ex(const GC89P_Runtime *runtime,
                  const GC89_Core *core,
                  int fallback_screen_width,
                  int fallback_screen_height,
                  const GC89_DrawSpec *spec,
                  const GC89P_DrawMeta *meta,
                  GC89P_DrawReport *report)
{
    int width;
    int height;
    int cx;
    int cy;
    int emitted;
    int provider_emitted;
    int handled;
    int hud_used;
    int image_w;
    int image_h;
    int image_x;
    int image_y;
    GC89P_VectorCommand command;

    if (report) memset(report, 0, sizeof(*report));
    if (!runtime || !core || !spec) return 0;
    if (!core->visible || !spec->visible) return 0;

    hud_used = 0;
    gc89p_resolve_viewport(runtime,
                           fallback_screen_width,
                           fallback_screen_height,
                           &width, &height, &hud_used);
    if (width <= 0 || height <= 0) return 0;
    gc89p_resolve_center(runtime, core, spec,
                         width, height, &cx, &cy, &hud_used);

    if (runtime->hud_provider.begin) {
        runtime->hud_provider.begin(runtime->hud_user, cx, cy, spec);
        hud_used = 1;
    }

    emitted = 0;
    if ((spec->draw_mode & GC89_DRAW_VECTOR) != 0) {
        handled = GC89P_UNHANDLED;
        provider_emitted = 0;
        if (runtime->vector_provider.draw_vector) {
            command.screen_width = width;
            command.screen_height = height;
            command.center_x = cx;
            command.center_y = cy;
            command.scale_fx = core->scale_fx;
            command.spec = spec;
            command.meta = meta;
            handled = runtime->vector_provider.draw_vector(
                runtime->vector_user, &command, &provider_emitted);
        }
        if (handled == GC89P_HANDLED) {
            if (provider_emitted < 0) provider_emitted = 0;
            emitted += provider_emitted;
            if (report) report->vector_provider_used = 1;
        } else if (runtime->allow_internal_vector_fallback) {
            provider_emitted = gc89p_draw_internal_vector(runtime, core,
                                                          width, height,
                                                          cx, cy, spec);
            emitted += provider_emitted;
            if (report && provider_emitted > 0)
                report->internal_vector_used = 1;
        }
    }

    if ((spec->draw_mode & GC89_DRAW_IMAGE) != 0) {
        image_w = gc89p_scaled_px(spec->image_width_fx, core->scale_fx, 1);
        image_h = gc89p_scaled_px(spec->image_height_fx, core->scale_fx, 1);
        image_x = cx - image_w / 2;
        image_y = cy - image_h / 2;
        handled = GC89P_UNHANDLED;
        if (runtime->asset_provider.draw_asset) {
            handled = runtime->asset_provider.draw_asset(
                runtime->asset_user,
                spec->image_id,
                image_x, image_y,
                image_w, image_h,
                spec->image_tint_rgba);
        }
        if (handled == GC89P_HANDLED) {
            emitted++;
            if (report) report->asset_provider_used = 1;
        } else if (gc89p_have_image(runtime)) {
            GC89P_PrimitiveMux mux;
            mux.runtime = runtime;
            mux.translate_x = 0;
            mux.translate_y = 0;
            gc89p_mux_image(&mux, spec->image_id,
                            image_x, image_y,
                            image_w, image_h,
                            spec->image_tint_rgba);
            emitted++;
            if (report) report->primitive_image_used = 1;
        }
    }

    if (runtime->hud_provider.end) {
        runtime->hud_provider.end(runtime->hud_user, emitted);
        hud_used = 1;
    }

    if (report) {
        report->center_x = cx;
        report->center_y = cy;
        report->emitted_primitives = emitted;
        report->hud_provider_used = hud_used;
    }
    return emitted;
}

int gc89p_draw(const GC89P_Runtime *runtime,
               const GC89_Core *core,
               int fallback_screen_width,
               int fallback_screen_height,
               const GC89_DrawSpec *spec,
               GC89P_DrawReport *report)
{
    return gc89p_draw_ex(runtime, core,
                         fallback_screen_width,
                         fallback_screen_height,
                         spec, 0, report);
}

static void gc89p_surface_put(GC89P_SoftwareSurface *surface,
                              int x, int y,
                              unsigned long color)
{
    unsigned char *p;
    unsigned int src_a;
    unsigned int inv_a;
    unsigned int dst_r;
    unsigned int dst_g;
    unsigned int dst_b;
    unsigned int dst_a;
    unsigned int src_r;
    unsigned int src_g;
    unsigned int src_b;

    if (!surface || !surface->rgba) return;
    if (x < 0 || y < 0 || x >= surface->width || y >= surface->height) return;
    p = surface->rgba + y * surface->stride_bytes + x * 4;
    src_r = (unsigned int)GC89_RGBA_R(color);
    src_g = (unsigned int)GC89_RGBA_G(color);
    src_b = (unsigned int)GC89_RGBA_B(color);
    src_a = (unsigned int)GC89_RGBA_A(color);
    if (src_a >= 255U) {
        p[0] = (unsigned char)src_r;
        p[1] = (unsigned char)src_g;
        p[2] = (unsigned char)src_b;
        p[3] = 255U;
        return;
    }
    inv_a = 255U - src_a;
    dst_r = p[0];
    dst_g = p[1];
    dst_b = p[2];
    dst_a = p[3];
    p[0] = (unsigned char)((src_r * src_a + dst_r * inv_a + 127U) / 255U);
    p[1] = (unsigned char)((src_g * src_a + dst_g * inv_a + 127U) / 255U);
    p[2] = (unsigned char)((src_b * src_a + dst_b * inv_a + 127U) / 255U);
    p[3] = (unsigned char)(src_a + (dst_a * inv_a + 127U) / 255U);
}

static void gc89p_surface_brush(GC89P_SoftwareSurface *surface,
                                int x, int y,
                                int thickness,
                                unsigned long color)
{
    int half;
    int start;
    int end;
    int ox;
    int oy;
    if (thickness < 1) thickness = 1;
    half = thickness / 2;
    start = -half;
    end = start + thickness - 1;
    for (oy = start; oy <= end; ++oy) {
        for (ox = start; ox <= end; ++ox) {
            gc89p_surface_put(surface, x + ox, y + oy, color);
        }
    }
}

static void gc89p_soft_line(void *user,
                            int x0, int y0, int x1, int y1,
                            int thickness_px,
                            unsigned long color_rgba)
{
    GC89P_SoftwareSurface *surface;
    int dx;
    int sx;
    int dy;
    int sy;
    int err;
    int e2;
    surface = (GC89P_SoftwareSurface *)user;
    dx = x1 >= x0 ? x1 - x0 : x0 - x1;
    sx = x0 < x1 ? 1 : -1;
    dy = y1 >= y0 ? y0 - y1 : y1 - y0;
    sy = y0 < y1 ? 1 : -1;
    err = dx + dy;
    for (;;) {
        gc89p_surface_brush(surface, x0, y0, thickness_px, color_rgba);
        if (x0 == x1 && y0 == y1) break;
        e2 = err + err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void gc89p_soft_dot(void *user,
                           int center_x, int center_y,
                           int diameter_px,
                           unsigned long color_rgba)
{
    GC89P_SoftwareSurface *surface;
    int radius;
    int r2;
    int x;
    int y;
    int dx;
    int dy;
    surface = (GC89P_SoftwareSurface *)user;
    if (diameter_px < 1) diameter_px = 1;
    radius = diameter_px / 2;
    if (radius < 1) {
        gc89p_surface_put(surface, center_x, center_y, color_rgba);
        return;
    }
    r2 = radius * radius;
    for (y = center_y - radius; y <= center_y + radius; ++y) {
        dy = y - center_y;
        for (x = center_x - radius; x <= center_x + radius; ++x) {
            dx = x - center_x;
            if (dx * dx + dy * dy <= r2)
                gc89p_surface_put(surface, x, y, color_rgba);
        }
    }
}

void gc89p_surface_init(GC89P_SoftwareSurface *surface,
                        unsigned char *rgba,
                        int width,
                        int height,
                        int stride_bytes)
{
    if (!surface) return;
    surface->rgba = rgba;
    surface->width = width;
    surface->height = height;
    surface->stride_bytes = stride_bytes;
    if (surface->stride_bytes < width * 4)
        surface->stride_bytes = width * 4;
}

void gc89p_surface_clear(GC89P_SoftwareSurface *surface,
                         unsigned long rgba)
{
    int x;
    int y;
    if (!surface || !surface->rgba) return;
    for (y = 0; y < surface->height; ++y) {
        for (x = 0; x < surface->width; ++x) {
            unsigned char *p;
            p = surface->rgba + y * surface->stride_bytes + x * 4;
            p[0] = (unsigned char)GC89_RGBA_R(rgba);
            p[1] = (unsigned char)GC89_RGBA_G(rgba);
            p[2] = (unsigned char)GC89_RGBA_B(rgba);
            p[3] = (unsigned char)GC89_RGBA_A(rgba);
        }
    }
}

void gc89p_make_software_primitives(GC89_DrawCallbacks *callbacks)
{
    if (!callbacks) return;
    callbacks->draw_line = gc89p_soft_line;
    callbacks->draw_dot = gc89p_soft_dot;
    callbacks->draw_image = 0;
}

int gc89p_draw_rgba(const GC89_Core *core,
                    const GC89_DrawSpec *spec,
                    GC89P_SoftwareSurface *surface,
                    GC89P_DrawReport *report)
{
    GC89P_Runtime runtime;
    GC89_DrawCallbacks software;
    if (!surface) return 0;
    gc89p_runtime_init(&runtime);
    gc89p_make_software_primitives(&software);
    gc89p_runtime_set_primitive_fallback(&runtime, &software, surface);
    return gc89p_draw(&runtime, core,
                      surface->width, surface->height,
                      spec, report);
}
