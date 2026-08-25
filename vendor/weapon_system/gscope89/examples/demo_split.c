#include <stdio.h>
#include "../include/gscope89_common.h"
#include "../gscopeini89/include/gscopeini89.h"
#include "../gtelescopiczoom89/include/gtelescopiczoom89.h"
#include "../gsway89/include/gsway89.h"
#include "../gsniperhud89/include/gsniperhud89.h"
#include "../gaimquery89/include/gaimquery89.h"

static g89_fx demo_fx_mul(g89_fx a, g89_fx b)
{
    return (g89_fx)((a / 256L) * (b / 256L));
}

static int demo_raycast(void *user,
                        const g89_vec3 *from,
                        const g89_vec3 *dir,
                        g89_fx max_dist,
                        gaq89_hit *out_hit)
{
    (void)user;
    (void)max_dist;
    out_hit->distance = G89_FX_FROM_INT(32);
    out_hit->position.x = from->x + demo_fx_mul(dir->x, out_hit->distance);
    out_hit->position.y = from->y + demo_fx_mul(dir->y, out_hit->distance);
    out_hit->position.z = from->z + demo_fx_mul(dir->z, out_hit->distance);
    out_hit->normal.x = 0;
    out_hit->normal.y = 0;
    out_hit->normal.z = -G89_FX_ONE;
    out_hit->target_id = 7;
    out_hit->target_kind = GAQ89_TARGET_ACTOR;
    out_hit->target_part = 1;
    out_hit->material_id = 3;
    out_hit->flags = 0;
    return 1;
}

static void demo_hud_emit(void *user, const gsh89_cmd *cmd)
{
    (void)user;
    printf("hud_cmd kind=%d id=%d xy=(%d,%d,%d,%d) values=(%ld,%ld,%ld,%ld)\n",
           cmd->kind, cmd->id,
           cmd->x0, cmd->y0, cmd->x1, cmd->y1,
           cmd->value0, cmd->value1, cmd->value2, cmd->value3);
}

int main(void)
{
    g89_camera camera;
    gtz89_ctx zoom;
    gtz89_profile zoom_profile;
    gsw89_ctx sway;
    gsw89_profile sway_profile;
    gsh89_ctx hud;
    gsh89_profile hud_profile;
    gsh89_telemetry telemetry;
    gaq89_ctx aim;
    const gaq89_hit *hit;
    short frame;
    gri89_doc recipe;

    camera.pos.x = G89_FX_FROM_INT(0);
    camera.pos.y = G89_FX_FROM_INT(2);
    camera.pos.z = G89_FX_FROM_INT(-10);
    camera.forward.x = 0;
    camera.forward.y = 0;
    camera.forward.z = G89_FX_ONE;
    camera.right.x = G89_FX_ONE;
    camera.right.y = 0;
    camera.right.z = 0;
    camera.up.x = 0;
    camera.up.y = G89_FX_ONE;
    camera.up.z = 0;
    camera.base_fov_deg_x100 = 6000;
    camera.current_fov_deg_x100 = 6000;

    gri89_init(&recipe);
    if (!gri89_load(&recipe, "config/hud/sniper_default.ini")) {
        printf("recipe_load_failed code=%d line=%d\n",
               recipe.error_code, recipe.error_line);
        return 2;
    }
    if (!gtz89_profile_from_recipe(&recipe, &zoom_profile) ||
        !gsw89_profile_from_recipe(&recipe, &sway_profile) ||
        !gsh89_profile_from_recipe(&recipe, &hud_profile)) {
        printf("recipe_profile_failed\n");
        return 3;
    }

    gtz89_init(&zoom, &zoom_profile, camera.base_fov_deg_x100);
    gsw89_init(&sway, &sway_profile);
    gsh89_init(&hud, &hud_profile, 320, 240, demo_hud_emit, 0);
    gaq89_init(&aim, demo_raycast, 0);

    gtz89_begin(&zoom);
    for (frame = 0; frame < 20; ++frame) {
        gsw89_set_hold(&sway, (short)(frame >= 8));
        gsw89_set_movement_pct(&sway, (short)(frame < 4 ? 35 : 0));
        gsw89_set_stress_pct(&sway, 20);
        camera.current_fov_deg_x100 = gtz89_update(&zoom, 1);
        gsw89_update(&sway, 1);
    }

    gaq89_query_center(&aim, &camera, G89_FX_FROM_INT(128));
    hit = gaq89_get_last_hit(&aim);

    gsh89_set_visibility(&hud, (short)(zoom.blend_x1000 > 0),
                         zoom.blend_x1000);
    gsh89_emit_overlay(&hud);

    telemetry.current_fov_deg_x100 = zoom.current_fov_deg_x100;
    telemetry.zoom_x100 = zoom.profile.zoom_x100;
    telemetry.sensitivity_pct = zoom.sensitivity_pct;
    telemetry.sway_yaw_deg_x1000 = sway.yaw_out_deg_x1000;
    telemetry.sway_pitch_deg_x1000 = sway.pitch_out_deg_x1000;
    telemetry.breath_wave_x1000 = sway.breath_wave_x1000;
    telemetry.hold_remaining_pct = sway.hold_remaining_pct;
    telemetry.breath_exhausted = sway.exhausted;
    telemetry.target_valid = hit ? hit->valid : 0;
    telemetry.target_id = hit ? hit->target_id : -1;
    telemetry.target_kind = hit ? hit->target_kind : 0;
    telemetry.target_part = hit ? hit->target_part : -1;
    telemetry.target_distance = hit ? hit->distance : 0;
    if (hit) telemetry.impact_position = hit->position;
    else {
        telemetry.impact_position.x = 0;
        telemetry.impact_position.y = 0;
        telemetry.impact_position.z = 0;
    }
    gsh89_emit_telemetry(&hud, &telemetry);

    printf("recipe_scope=%s recipe_zoom=%s recipe_sway=%s\n",
           gri89_get(&recipe, "scope", "use", "?"),
           gri89_get(&recipe, "zoom", "use", "?"),
           gri89_get(&recipe, "sway", "use", "?"));

    printf("zoom_fov=%d.%02d sens=%d sway=(%d,%d) hold=%d%% ",
           zoom.current_fov_deg_x100 / 100,
           zoom.current_fov_deg_x100 % 100,
           zoom.sensitivity_pct,
           sway.yaw_out_deg_x1000,
           sway.pitch_out_deg_x1000,
           sway.hold_remaining_pct);
    if (hit && hit->valid) {
        printf("target=%d kind=%d dist=%d impact=(%d,%d,%d)\n",
               hit->target_id,
               hit->target_kind,
               G89_FX_TO_INT(hit->distance),
               G89_FX_TO_INT(hit->position.x),
               G89_FX_TO_INT(hit->position.y),
               G89_FX_TO_INT(hit->position.z));
    } else {
        printf("no_hit\n");
    }

    return 0;
}
