/*
    cameranaku89_composer.h
    CC0 1.0 Universal.
    Optional screen-composition helper for shot quality and safe framing.
*/
#ifndef CAMERANAKU89_COMPOSER_H
#define CAMERANAKU89_COMPOSER_H

#include "cameranaku89.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CNK_COMPOSER_CENTER = 0,
    CNK_COMPOSER_RULE_OF_THIRDS_LEFT = 1,
    CNK_COMPOSER_RULE_OF_THIRDS_RIGHT = 2,
    CNK_COMPOSER_LOW_ANGLE = 3,
    CNK_COMPOSER_HIGH_ANGLE = 4
};

typedef struct cnk_composer_s {
    int enabled;
    int rule;
    cnk_fx desired_x;      /* normalized 0..1 */
    cnk_fx desired_y;      /* normalized 0..1 */
    cnk_fx dead_x;         /* normalized radius */
    cnk_fx dead_y;
    cnk_fx soft_x;
    cnk_fx soft_y;
    cnk_fx center_weight;
    cnk_fx offscreen_penalty;
    cnk_fx obstruction_penalty;
    cnk_fx viewport_push_limit;
} cnk_composer;

typedef struct cnk_composer_report_s {
    cnk_fx quality_delta;
    cnk_fx screen_x;
    cnk_fx screen_y;
    cnk_fx error_x;
    cnk_fx error_y;
    int on_screen;
    int inside_dead_zone;
} cnk_composer_report;

CNK_API void cnk_composer_default(cnk_composer *composer);
CNK_API void cnk_composer_set_rule(cnk_composer *composer, int rule);
CNK_API void cnk_composer_set_target_screen(cnk_composer *composer, cnk_fx x, cnk_fx y);
CNK_API void cnk_composer_set_zones(cnk_composer *composer, cnk_fx dead_x, cnk_fx dead_y, cnk_fx soft_x, cnk_fx soft_y);
CNK_API cnk_composer_report cnk_composer_score_camera(const cnk_composer *composer, const cnk_camera *camera, cnk_vec3 target_pos, int obstructed);
CNK_API int cnk_composer_apply_viewport_offset(const cnk_composer *composer, cnk_state *state, const cnk_composer_report *report);

#ifdef __cplusplus
}
#endif

#endif
