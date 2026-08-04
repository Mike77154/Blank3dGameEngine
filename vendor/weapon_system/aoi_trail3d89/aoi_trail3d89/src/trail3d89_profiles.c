#include "trail3d89_profiles.h"

void t3d89_profile_bullet(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_VIEW_RIBBON;
    desc->life_ticks = 8;
    desc->max_points = 32;
    desc->sampler_mask = T3D89_SAMPLE_DISTANCE | T3D89_SAMPLE_CURVE;
    desc->min_dist = T3D89_FP_ONE / 6;
    desc->curve_dist = T3D89_FP_ONE / 4;
    desc->width_head = T3D89_FP_ONE / 8;
    desc->width_tail = 0;
    desc->color_head = t3d89_color_make(255, 240, 64, 230);
    desc->color_tail = t3d89_color_make(255, 80, 0, 0);
    desc->uv_mode = T3D89_UV_BY_DISTANCE;
    desc->uv_tile_dist = T3D89_FP_ONE;
    desc->lod_vertex_budget = 32;
}

void t3d89_profile_tracer(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_TUBE_LITE;
    desc->life_ticks = 10;
    desc->max_points = 24;
    desc->sampler_mask = T3D89_SAMPLE_DISTANCE;
    desc->min_dist = T3D89_FP_ONE / 3;
    desc->width_head = T3D89_FP_ONE / 10;
    desc->width_tail = 0;
    desc->color_head = t3d89_color_make(255, 250, 120, 220);
    desc->color_tail = t3d89_color_make(255, 100, 0, 0);
    desc->lod_vertex_budget = 48;
}

void t3d89_profile_dash_ground(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_AXIS_RIBBON;
    desc->life_ticks = 16;
    desc->max_points = 48;
    desc->sampler_mask = T3D89_SAMPLE_DISTANCE | T3D89_SAMPLE_TIME | T3D89_SAMPLE_CURVE;
    desc->min_ticks = 1;
    desc->min_dist = T3D89_FP_ONE / 3;
    desc->curve_dist = T3D89_FP_ONE / 3;
    desc->width_head = T3D89_FP_ONE;
    desc->width_tail = T3D89_FP_ONE / 8;
    desc->axis_x = 0;
    desc->axis_y = T3D89_FP_ONE;
    desc->axis_z = 0;
    desc->color_head = t3d89_color_make(70, 180, 255, 190);
    desc->color_tail = t3d89_color_make(70, 80, 255, 0);
    desc->lod_vertex_budget = 64;
}

void t3d89_profile_dash_air(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_CROSS_RIBBON;
    desc->life_ticks = 14;
    desc->max_points = 48;
    desc->sampler_mask = T3D89_SAMPLE_DISTANCE | T3D89_SAMPLE_CURVE;
    desc->min_dist = T3D89_FP_ONE / 3;
    desc->curve_dist = T3D89_FP_ONE / 3;
    desc->width_head = (T3D89_FP_ONE * 3) / 4;
    desc->width_tail = 0;
    desc->color_head = t3d89_color_make(90, 220, 255, 180);
    desc->color_tail = t3d89_color_make(100, 60, 255, 0);
    desc->lod_vertex_budget = 96;
}

void t3d89_profile_casing(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_CROSS_RIBBON;
    desc->life_ticks = 18;
    desc->max_points = 24;
    desc->sampler_mask = T3D89_SAMPLE_DISTANCE | T3D89_SAMPLE_TIME;
    desc->min_ticks = 1;
    desc->min_dist = T3D89_FP_ONE / 5;
    desc->width_head = T3D89_FP_ONE / 12;
    desc->width_tail = 0;
    desc->color_head = t3d89_color_make(255, 210, 100, 130);
    desc->color_tail = t3d89_color_make(180, 100, 40, 0);
    desc->lod_vertex_budget = 48;
}

void t3d89_profile_beam(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_BEAM_AB;
    desc->life_ticks = 2;
    desc->max_points = 4;
    desc->sampler_mask = T3D89_SAMPLE_FORCE;
    desc->width_head = T3D89_FP_ONE / 8;
    desc->width_tail = T3D89_FP_ONE / 16;
    desc->color_head = t3d89_color_make(255, 255, 255, 230);
    desc->color_tail = t3d89_color_make(80, 180, 255, 0);
    desc->lod_vertex_budget = 16;
}

void t3d89_profile_socket_sword(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_SOCKET_SWEEP;
    desc->life_ticks = 8;
    desc->max_points = 16;
    desc->sampler_mask = T3D89_SAMPLE_FORCE;
    desc->width_head = T3D89_FP_ONE / 4;
    desc->width_tail = 0;
    desc->color_head = t3d89_color_make(255, 255, 255, 210);
    desc->color_tail = t3d89_color_make(90, 190, 255, 0);
    desc->lod_vertex_budget = 32;
}

void t3d89_profile_claw(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_SOCKET_SWEEP;
    desc->life_ticks = 6;
    desc->max_points = 12;
    desc->sampler_mask = T3D89_SAMPLE_FORCE;
    desc->width_head = T3D89_FP_ONE / 6;
    desc->width_tail = 0;
    desc->color_head = t3d89_color_make(255, 230, 220, 180);
    desc->color_tail = t3d89_color_make(255, 60, 40, 0);
    desc->lod_vertex_budget = 24;
}

void t3d89_profile_energy_cross(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_CROSS_RIBBON;
    desc->life_ticks = 24;
    desc->max_points = 64;
    desc->sampler_mask = T3D89_SAMPLE_DISTANCE | T3D89_SAMPLE_TIME | T3D89_SAMPLE_CURVE;
    desc->min_ticks = 2;
    desc->min_dist = T3D89_FP_ONE / 4;
    desc->curve_dist = T3D89_FP_ONE / 3;
    desc->width_head = T3D89_FP_ONE / 2;
    desc->width_tail = 0;
    desc->color_head = t3d89_color_make(150, 220, 255, 160);
    desc->color_tail = t3d89_color_make(70, 80, 255, 0);
    desc->lod_vertex_budget = 96;
}

void t3d89_profile_debug_path(t3d89_desc *desc)
{
    t3d89_default_desc(desc);
    if (desc == 0) {
        return;
    }
    desc->mode = T3D89_MODE_VIEW_RIBBON;
    desc->life_ticks = 200;
    desc->max_points = 128;
    desc->sampler_mask = T3D89_SAMPLE_DISTANCE | T3D89_SAMPLE_CURVE;
    desc->min_dist = T3D89_FP_ONE;
    desc->curve_dist = T3D89_FP_ONE;
    desc->width_head = T3D89_FP_ONE / 16;
    desc->width_tail = T3D89_FP_ONE / 16;
    desc->color_head = t3d89_color_make(80, 255, 120, 160);
    desc->color_tail = t3d89_color_make(80, 255, 120, 20);
    desc->lod_vertex_budget = 128;
}
