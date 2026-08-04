#include <stdio.h>
#include <string.h>

#include "blank3d_perception_ini.h"

static int verify_file(const char *path)
{
    Blank3DTruthGate gate;
    Blank3DPerceptionConfig config;
    g3d_fix truth_range;
    char status[128];

    blank3d_truth_gate_init(&gate, B3D_TRUTH_PROFILE_RETRO);
    blank3d_perception_config_defaults(&config);
    truth_range = G3D_FIX_FROM_INT(28);
    status[0] = '\0';

    if (!blank3d_perception_ini_load(path, &gate, &truth_range,
            &config, status, sizeof(status))) return 0;
    if (gate.profile != B3D_TRUTH_PROFILE_STRICT) return 0;
    if (truth_range != G3D_FIX_FROM_INT(30)) return 0;
    if (config.view_range != G3D_FIX_FROM_INT(30)) return 0;
    if (config.eye_shape != B3D_EYE_SHAPE_CONE) return 0;
    if (config.horizontal_fov_degrees != 100) return 0;
    if (config.vertical_fov_degrees != 70) return 0;
    if (config.hearing_range != G3D_FIX_FROM_INT(24)) return 0;
    if (!config.require_line_of_sight) return 0;
    if (strcmp(status, "perception INI loaded") != 0) return 0;
    return 1;
}

int main(void)
{
    if (!verify_file("config/entities/zombie.ini")) return 1;
    if (!verify_file("config/entities/gunner_enemy.ini")) return 2;
    if (!verify_file("config/entities/hopper_enemy.ini")) return 3;
    if (!verify_file("config/entities/dive_enemy.ini")) return 4;
    if (!verify_file("config/entities/air_lunger_enemy.ini")) return 5;
    if (!verify_file("config/entities/ground_lancer_enemy.ini")) return 6;
    puts("Entity perception INI loading: OK");
    return 0;
}
