#include "ecg_renderer.h"
#include "ecg_fixed.h"
#include "ecg_surface.h"

static int ecg_abs_int(int value)
{
    return value < 0 ? -value : value;
}

void ecg_render_config_default(ECG_RenderConfig *config_out)
{
    if (config_out == (ECG_RenderConfig *)0) {
        return;
    }

    config_out->draw_flags = ECG_RENDER_DRAW_ALL;
    config_out->x_step_q8 = ecg_fixed_from_int(3);
    config_out->y_step_q8 = ecg_fixed_from_int(3);
    config_out->bar_width_px = 2u;
    config_out->grid_x_units = 8u;
    config_out->grid_y_units = 5u;
    config_out->axis_y_units = 15u;
    config_out->waveform_y_units = ECG_WAVEFORM_Y_UNITS_DEFAULT;
    config_out->grid_color = ecg_color_make(28u, 34u, 32u);
    config_out->axis_color = ecg_color_make(45u, 60u, 50u);
    config_out->frame_color = ecg_color_make(52u, 64u, 58u);
    config_out->window_color = ecg_color_make(230u, 230u, 230u);

    config_out->signal_style = ECG_SIGNAL_STYLE_CONTINUOUS;
    config_out->dot_on_px = 2u;
    config_out->dot_off_px = 2u;
    config_out->glow_enabled = 0u;
    config_out->glow_radius_px = 2u;
    config_out->glow_intensity = 72u;
    config_out->glow_color = ecg_color_make(255u, 255u, 255u);
    config_out->use_profile_glow_color = 1u;
}

static ECG_RenderConfig ecg_get_config_or_default(const ECG_RenderConfig *config)
{
    ECG_RenderConfig local;

    if (config == (const ECG_RenderConfig *)0) {
        ecg_render_config_default(&local);
    } else {
        local = *config;
    }

    if (local.x_step_q8 <= (ECG_FixedQ8)0) {
        local.x_step_q8 = ecg_fixed_from_int(1);
    }
    if (local.y_step_q8 <= (ECG_FixedQ8)0) {
        local.y_step_q8 = ecg_fixed_from_int(1);
    }
    if (local.bar_width_px == 0u) {
        local.bar_width_px = 1u;
    }
    if (local.grid_x_units == 0u) {
        local.grid_x_units = 1u;
    }
    if (local.grid_y_units == 0u) {
        local.grid_y_units = 1u;
    }
    if (local.waveform_y_units == 0u) {
        local.waveform_y_units = ECG_WAVEFORM_Y_UNITS_DEFAULT;
    }
    if (local.signal_style > ECG_SIGNAL_STYLE_BARS) {
        local.signal_style = ECG_SIGNAL_STYLE_CONTINUOUS;
    }
    if (local.dot_on_px == 0u) {
        local.dot_on_px = 1u;
    }
    if (local.dot_off_px == 0u) {
        local.dot_off_px = 1u;
    }
    if (local.glow_intensity > 255u) {
        local.glow_intensity = 255u;
    }

    return local;
}

static void ecg_draw_glow_stamp(ECG_Surface *surface,
                                int x,
                                int y,
                                ECG_Color glow_color,
                                const ECG_RenderConfig *config)
{
    int dx;
    int dy;
    int radius;
    int distance;
    unsigned int numerator;
    unsigned int denominator;
    ECG_Color layer;

    if (config->glow_enabled == 0u ||
        config->glow_radius_px == 0u ||
        config->glow_intensity == 0u) {
        return;
    }

    radius = (int)config->glow_radius_px;
    denominator = 255u * (config->glow_radius_px + 1u);

    for (dy = -radius; dy <= radius; dy++) {
        for (dx = -radius; dx <= radius; dx++) {
            distance = ecg_abs_int(dx) + ecg_abs_int(dy);
            if (distance <= radius) {
                numerator = config->glow_intensity *
                            (unsigned int)(radius - distance + 1);
                layer = ecg_color_scale(glow_color,
                                        numerator,
                                        denominator);
                ecg_put_pixel_lighten(surface, x + dx, y + dy, layer);
            }
        }
    }
}

static void ecg_draw_core_stamp(ECG_Surface *surface,
                                int x,
                                int y,
                                ECG_Color color,
                                unsigned int width)
{
    int left;
    int top;

    if (width == 0u) {
        width = 1u;
    }

    left = x - (int)((width - 1u) / 2u);
    top = y - (int)((width - 1u) / 2u);
    (void)ecg_fill_rect(surface, left, top, width, width, color);
}

static void ecg_draw_styled_segment(ECG_Surface *surface,
                                    int x0,
                                    int y0,
                                    int x1,
                                    int y1,
                                    ECG_Color color,
                                    ECG_Color glow_color,
                                    const ECG_RenderConfig *config,
                                    unsigned int *phase_io,
                                    unsigned int force_continuous)
{
    int dx;
    int sx;
    int dy;
    int sy;
    int error;
    int twice_error;
    unsigned int phase;
    unsigned int period;
    unsigned int draw_point;

    dx = ecg_abs_int(x1 - x0);
    sx = x0 < x1 ? 1 : -1;
    dy = -ecg_abs_int(y1 - y0);
    sy = y0 < y1 ? 1 : -1;
    error = dx + dy;
    phase = phase_io == (unsigned int *)0 ? 0u : *phase_io;
    period = config->dot_on_px + config->dot_off_px;
    if (period == 0u) {
        period = 1u;
    }

    while (1) {
        draw_point = 1u;
        if (force_continuous == 0u &&
            config->signal_style == ECG_SIGNAL_STYLE_DOTTED) {
            draw_point = (phase % period) < config->dot_on_px ? 1u : 0u;
        }

        if (draw_point != 0u) {
            ecg_draw_glow_stamp(surface, x0, y0, glow_color, config);
            ecg_draw_core_stamp(surface,
                                x0,
                                y0,
                                color,
                                config->bar_width_px);
        }

        phase++;
        if (x0 == x1 && y0 == y1) {
            break;
        }

        twice_error = error * 2;
        if (twice_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }

    if (phase_io != (unsigned int *)0) {
        *phase_io = phase;
    }
}

ECG_Status ecg_draw_grid(ECG_Surface *surface, int x, int y,
                         unsigned int column_count,
                         const ECG_RenderConfig *config)
{
    ECG_RenderConfig cfg;
    int grid_w;
    int grid_h;
    int step_x;
    int step_y;
    int axis_y;
    int i;

    if (!ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }
    if (column_count == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    cfg = ecg_get_config_or_default(config);
    grid_w = ecg_fixed_mul_uint_to_int_floor(column_count, cfg.x_step_q8);
    grid_h = ecg_fixed_mul_uint_to_int_floor(cfg.waveform_y_units,
                                             cfg.y_step_q8);
    step_x = ecg_fixed_mul_uint_to_int_floor(cfg.grid_x_units,
                                             cfg.x_step_q8);
    step_y = ecg_fixed_mul_uint_to_int_floor(cfg.grid_y_units,
                                             cfg.y_step_q8);
    axis_y = ecg_fixed_mul_uint_to_int_floor(cfg.axis_y_units,
                                             cfg.y_step_q8);

    if (step_x < 1) {
        step_x = 1;
    }
    if (step_y < 1) {
        step_y = 1;
    }
    if (grid_w < 1 || grid_h < 1) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    if ((cfg.draw_flags & ECG_RENDER_DRAW_GRID) != 0ul) {
        for (i = 0; i <= grid_w; i += step_x) {
            (void)ecg_draw_vline(surface,
                                 x + i,
                                 y,
                                 y + grid_h,
                                 cfg.grid_color);
        }
        for (i = 0; i <= grid_h; i += step_y) {
            (void)ecg_draw_hline(surface,
                                 x,
                                 x + grid_w,
                                 y + i,
                                 cfg.grid_color);
        }
    }

    if ((cfg.draw_flags & ECG_RENDER_DRAW_AXIS) != 0ul) {
        (void)ecg_draw_hline(surface,
                             x,
                             x + grid_w,
                             y + axis_y,
                             cfg.axis_color);
    }

    return ECG_STATUS_OK;
}

ECG_Status ecg_draw_column_bar(ECG_Surface *surface, int x, int top_y,
                               ECG_Line line, ECG_Color color,
                               const ECG_RenderConfig *config)
{
    ECG_RenderConfig cfg;
    unsigned int height_units;
    int y0;
    int y1;
    int height_px;
    unsigned int phase;
    ECG_Color glow_color;

    if (!ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }

    cfg = ecg_get_config_or_default(config);
    if (line.h < (ECG_I8)0) {
        height_units = (unsigned int)(-(int)line.h);
    } else {
        height_units = (unsigned int)line.h;
    }

    y0 = top_y + ecg_fixed_mul_i8_to_int_floor(line.y, cfg.y_step_q8);
    height_px = ecg_fixed_mul_uint_to_int_ceil(height_units,
                                               cfg.y_step_q8);
    if (height_px < 1) {
        height_px = 1;
    }
    y1 = y0 + height_px;
    phase = 0u;
    glow_color = cfg.use_profile_glow_color != 0u ? color : cfg.glow_color;

    ecg_draw_styled_segment(surface,
                            x,
                            y0,
                            x,
                            y1,
                            color,
                            glow_color,
                            &cfg,
                            &phase,
                            1u);
    return ECG_STATUS_OK;
}

static void ecg_draw_profile_segment(ECG_Surface *surface,
                                     const ECG_Profile *profile,
                                     int x0,
                                     int y0,
                                     unsigned int left_index,
                                     unsigned int right_index,
                                     ECG_Color color,
                                     ECG_Color glow_color,
                                     const ECG_RenderConfig *config,
                                     unsigned int *phase_io)
{
    int sx0;
    int sy0;
    int sx1;
    int sy1;

    sx0 = x0 + ecg_fixed_mul_uint_to_int_floor(left_index,
                                                config->x_step_q8);
    sy0 = y0 + ecg_fixed_mul_i8_to_int_floor(profile->samples[left_index],
                                              config->y_step_q8);
    sx1 = x0 + ecg_fixed_mul_uint_to_int_floor(right_index,
                                                config->x_step_q8);
    sy1 = y0 + ecg_fixed_mul_i8_to_int_floor(profile->samples[right_index],
                                              config->y_step_q8);

    ecg_draw_styled_segment(surface,
                            sx0,
                            sy0,
                            sx1,
                            sy1,
                            color,
                            glow_color,
                            config,
                            phase_io,
                            0u);
}

static void ecg_draw_profile_range(ECG_Surface *surface,
                                   const ECG_Profile *profile,
                                   int x0,
                                   int y0,
                                   unsigned int left,
                                   unsigned int right,
                                   unsigned int fade_from_right,
                                   ECG_Color fixed_color,
                                   unsigned int use_fixed_color,
                                   const ECG_RenderConfig *config)
{
    unsigned int i;
    unsigned int age;
    unsigned int phase;
    ECG_Color color;
    ECG_Color glow_color;
    int point_x;
    int point_y;

    phase = 0u;
    if (left > right || right >= ECG_SIGNAL_COLS) {
        return;
    }

    if (config->signal_style == ECG_SIGNAL_STYLE_BARS) {
        for (i = left; i <= right; i++) {
            age = fade_from_right != 0u ? right - i : 0u;
            color = use_fixed_color != 0u ? fixed_color :
                    ecg_color_fade_step(profile->color,
                                        profile->gradient,
                                        age);
            (void)ecg_draw_column_bar(surface,
                                      x0 + ecg_fixed_mul_uint_to_int_floor(
                                                i,
                                                config->x_step_q8),
                                      y0,
                                      profile->lines[i],
                                      color,
                                      config);
        }
        return;
    }

    if (left == right) {
        color = use_fixed_color != 0u ? fixed_color : profile->color;
        glow_color = config->use_profile_glow_color != 0u ?
                     color : config->glow_color;
        point_x = x0 + ecg_fixed_mul_uint_to_int_floor(left,
                                                       config->x_step_q8);
        point_y = y0 + ecg_fixed_mul_i8_to_int_floor(profile->samples[left],
                                                      config->y_step_q8);
        ecg_draw_glow_stamp(surface, point_x, point_y, glow_color, config);
        ecg_draw_core_stamp(surface,
                            point_x,
                            point_y,
                            color,
                            config->bar_width_px);
        return;
    }

    for (i = left + 1u; i <= right; i++) {
        age = fade_from_right != 0u ? right - i : 0u;
        color = use_fixed_color != 0u ? fixed_color :
                ecg_color_fade_step(profile->color,
                                    profile->gradient,
                                    age);
        if (config->use_profile_glow_color != 0u) {
            glow_color = color;
        } else if (fade_from_right != 0u) {
            glow_color = ecg_color_fade_step(config->glow_color,
                                             profile->gradient,
                                             age);
        } else {
            glow_color = config->glow_color;
        }
        ecg_draw_profile_segment(surface,
                                 profile,
                                 x0,
                                 y0,
                                 i - 1u,
                                 i,
                                 color,
                                 glow_color,
                                 config,
                                 &phase);
    }
}

ECG_Status ecg_draw_active_window(ECG_Surface *surface,
                                  const ECG_Profile *profile,
                                  int x0, int y0,
                                  unsigned int offset,
                                  unsigned int visible_cols,
                                  const ECG_RenderConfig *config)
{
    ECG_RenderConfig cfg;
    unsigned int left;
    unsigned int right;
    int grid_w;
    int grid_h;

    if (!ecg_surface_is_valid(surface) || profile == (const ECG_Profile *)0) {
        return ECG_STATUS_NULL;
    }
    if (visible_cols == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    cfg = ecg_get_config_or_default(config);
    grid_w = ecg_fixed_mul_uint_to_int_floor(ECG_SIGNAL_COLS,
                                             cfg.x_step_q8);
    grid_h = ecg_fixed_mul_uint_to_int_floor(cfg.waveform_y_units,
                                             cfg.y_step_q8);

    (void)ecg_draw_grid(surface, x0, y0, ECG_SIGNAL_COLS, &cfg);
    if ((cfg.draw_flags & ECG_RENDER_DRAW_FRAME) != 0ul) {
        (void)ecg_draw_rect_outline(surface,
                                    x0 - 2,
                                    y0 - 2,
                                    (unsigned int)(grid_w + 5),
                                    (unsigned int)(grid_h + 5),
                                    cfg.frame_color);
    }

    if ((cfg.draw_flags & ECG_RENDER_DRAW_SIGNAL) == 0ul) {
        return ECG_STATUS_OK;
    }

    right = offset >= ECG_SIGNAL_COLS ? ECG_SIGNAL_COLS - 1u : offset;
    left = visible_cols > right + 1u ? 0u : right - visible_cols + 1u;
    ecg_draw_profile_range(surface,
                           profile,
                           x0,
                           y0,
                           left,
                           right,
                           1u,
                           profile->color,
                           0u,
                           &cfg);

    return ECG_STATUS_OK;
}

ECG_Status ecg_draw_overview(ECG_Surface *surface,
                             const ECG_Profile *profile,
                             int x0, int y0,
                             unsigned int offset,
                             unsigned int visible_cols,
                             const ECG_RenderConfig *config)
{
    ECG_RenderConfig cfg;
    unsigned int left;
    unsigned int right;
    ECG_Color color;
    int grid_w;
    int grid_h;
    int left_px;
    int right_px;

    if (!ecg_surface_is_valid(surface) || profile == (const ECG_Profile *)0) {
        return ECG_STATUS_NULL;
    }
    if (visible_cols == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    cfg = ecg_get_config_or_default(config);
    grid_w = ecg_fixed_mul_uint_to_int_floor(ECG_SIGNAL_COLS,
                                             cfg.x_step_q8);
    grid_h = ecg_fixed_mul_uint_to_int_floor(cfg.waveform_y_units,
                                             cfg.y_step_q8);

    (void)ecg_draw_grid(surface, x0, y0, ECG_SIGNAL_COLS, &cfg);
    if ((cfg.draw_flags & ECG_RENDER_DRAW_FRAME) != 0ul) {
        (void)ecg_draw_rect_outline(surface,
                                    x0 - 2,
                                    y0 - 2,
                                    (unsigned int)(grid_w + 5),
                                    (unsigned int)(grid_h + 5),
                                    cfg.frame_color);
    }

    if ((cfg.draw_flags & ECG_RENDER_DRAW_SIGNAL) != 0ul) {
        color = ecg_color_scale(profile->color, 5u, 8u);
        ecg_draw_profile_range(surface,
                               profile,
                               x0,
                               y0,
                               0u,
                               ECG_SIGNAL_COLS - 1u,
                               0u,
                               color,
                               1u,
                               &cfg);
    }

    right = offset >= ECG_SIGNAL_COLS ? ECG_SIGNAL_COLS - 1u : offset;
    left = visible_cols > right + 1u ? 0u : right - visible_cols + 1u;
    left_px = ecg_fixed_mul_uint_to_int_floor(left, cfg.x_step_q8);
    right_px = ecg_fixed_mul_uint_to_int_floor(right + 1u, cfg.x_step_q8);

    if ((cfg.draw_flags & ECG_RENDER_DRAW_WINDOW) != 0ul) {
        (void)ecg_draw_rect_outline(surface,
                                    x0 + left_px - 1,
                                    y0 - 1,
                                    (unsigned int)(right_px - left_px + 2),
                                    (unsigned int)(grid_h + 3),
                                    cfg.window_color);
    }

    return ECG_STATUS_OK;
}
