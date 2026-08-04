/* cameranaku89_composer.c - CC0 1.0 Universal. */
#include "cameranaku89_composer.h"

static cnk_fx cnk_comp_norm_from_px(int px, int size)
{
    if (size <= 0) {
        return 0;
    }
    return CNK_FX_FRAC(px, size);
}

static cnk_fx cnk_comp_abs(cnk_fx v)
{
    return v < 0 ? -v : v;
}

CNK_API void cnk_composer_default(cnk_composer *composer)
{
    if (composer == 0) {
        return;
    }
    composer->enabled = 1;
    composer->rule = CNK_COMPOSER_CENTER;
    composer->desired_x = CNK_HALF;
    composer->desired_y = CNK_HALF;
    composer->dead_x = CNK_FX_FRAC(7, 100);
    composer->dead_y = CNK_FX_FRAC(9, 100);
    composer->soft_x = CNK_FX_FRAC(28, 100);
    composer->soft_y = CNK_FX_FRAC(30, 100);
    composer->center_weight = CNK_FX_FROM_INT(500);
    composer->offscreen_penalty = CNK_FX_FROM_INT(900);
    composer->obstruction_penalty = CNK_FX_FROM_INT(700);
    composer->viewport_push_limit = CNK_FX_FRAC(20, 100);
}

CNK_API void cnk_composer_set_rule(cnk_composer *composer, int rule)
{
    if (composer == 0) {
        return;
    }
    composer->rule = rule;
    if (rule == CNK_COMPOSER_RULE_OF_THIRDS_LEFT) {
        composer->desired_x = CNK_FX_FRAC(1, 3);
        composer->desired_y = CNK_HALF;
    } else if (rule == CNK_COMPOSER_RULE_OF_THIRDS_RIGHT) {
        composer->desired_x = CNK_FX_FRAC(2, 3);
        composer->desired_y = CNK_HALF;
    } else if (rule == CNK_COMPOSER_LOW_ANGLE) {
        composer->desired_x = CNK_HALF;
        composer->desired_y = CNK_FX_FRAC(58, 100);
    } else if (rule == CNK_COMPOSER_HIGH_ANGLE) {
        composer->desired_x = CNK_HALF;
        composer->desired_y = CNK_FX_FRAC(42, 100);
    } else {
        composer->desired_x = CNK_HALF;
        composer->desired_y = CNK_HALF;
    }
}

CNK_API void cnk_composer_set_target_screen(cnk_composer *composer, cnk_fx x, cnk_fx y)
{
    if (composer == 0) {
        return;
    }
    composer->desired_x = cnk_fx_clamp(x, 0, CNK_ONE);
    composer->desired_y = cnk_fx_clamp(y, 0, CNK_ONE);
}

CNK_API void cnk_composer_set_zones(cnk_composer *composer, cnk_fx dead_x, cnk_fx dead_y, cnk_fx soft_x, cnk_fx soft_y)
{
    if (composer == 0) {
        return;
    }
    composer->dead_x = cnk_fx_clamp(dead_x, 0, CNK_ONE);
    composer->dead_y = cnk_fx_clamp(dead_y, 0, CNK_ONE);
    composer->soft_x = cnk_fx_clamp(soft_x, composer->dead_x, CNK_ONE);
    composer->soft_y = cnk_fx_clamp(soft_y, composer->dead_y, CNK_ONE);
}

CNK_API cnk_composer_report cnk_composer_score_camera(const cnk_composer *composer, const cnk_camera *camera, cnk_vec3 target_pos, int obstructed)
{
    cnk_composer_report report;
    int sx;
    int sy;
    cnk_fx depth;
    cnk_fx ax;
    cnk_fx ay;
    cnk_fx err;
    report.quality_delta = 0;
    report.screen_x = 0;
    report.screen_y = 0;
    report.error_x = 0;
    report.error_y = 0;
    report.on_screen = 0;
    report.inside_dead_zone = 0;
    if (composer == 0 || camera == 0 || composer->enabled == 0) {
        return report;
    }
    if (cnk_camera_project_point(camera, target_pos, &sx, &sy, &depth) == 0) {
        report.quality_delta -= composer->offscreen_penalty;
        if (obstructed != 0) {
            report.quality_delta -= composer->obstruction_penalty;
        }
        return report;
    }
    report.on_screen = 1;
    report.screen_x = cnk_comp_norm_from_px(sx, camera->state.lens.viewport_w);
    report.screen_y = cnk_comp_norm_from_px(sy, camera->state.lens.viewport_h);
    report.error_x = report.screen_x - composer->desired_x;
    report.error_y = report.screen_y - composer->desired_y;
    ax = cnk_comp_abs(report.error_x);
    ay = cnk_comp_abs(report.error_y);
    if (ax <= composer->dead_x && ay <= composer->dead_y) {
        report.inside_dead_zone = 1;
        report.quality_delta += composer->center_weight;
    } else {
        err = ax + ay;
        report.quality_delta += composer->center_weight;
        report.quality_delta -= cnk_fx_mul(err, composer->center_weight);
    }
    if (obstructed != 0) {
        report.quality_delta -= composer->obstruction_penalty;
    }
    return report;
}

CNK_API int cnk_composer_apply_viewport_offset(const cnk_composer *composer, cnk_state *state, const cnk_composer_report *report)
{
    cnk_fx push_x;
    cnk_fx push_y;
    if (composer == 0 || state == 0 || report == 0 || composer->enabled == 0) {
        return CNK_FALSE;
    }
    if (report->on_screen == 0 || report->inside_dead_zone != 0) {
        return CNK_FALSE;
    }
    push_x = -report->error_x;
    push_y = -report->error_y;
    push_x = cnk_fx_clamp(push_x, -composer->viewport_push_limit, composer->viewport_push_limit);
    push_y = cnk_fx_clamp(push_y, -composer->viewport_push_limit, composer->viewport_push_limit);
    state->lens.viewport_offset_x = cnk_fx_clamp(state->lens.viewport_offset_x + push_x, -composer->viewport_push_limit, composer->viewport_push_limit);
    state->lens.viewport_offset_y = cnk_fx_clamp(state->lens.viewport_offset_y + push_y, -composer->viewport_push_limit, composer->viewport_push_limit);
    return CNK_TRUE;
}
