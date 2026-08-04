#include "gfaction89.h"
#include "gfaction89_debug.h"
#include <stdio.h>

int main(void)
{
    GFA_World world;
    GFA_Context ctx;
    GFA_Decision d;
    int f_player;
    int f_bandits;
    int f_animals;
    int f_machines;
    int t_human;
    int t_machine;
    int player;
    int bandit;
    int dog;
    int turret;
    int candidates[4];
    int best;

    gfa_init(&world);
    f_player = gfa_register_faction(&world, "player");
    f_bandits = gfa_register_faction(&world, "bandits");
    f_animals = gfa_register_faction(&world, "animals");
    f_machines = gfa_register_faction(&world, "machines");
    t_human = gfa_register_tag(&world, "human");
    t_machine = gfa_register_tag(&world, "machine");

    gfa_set_relation(&world, f_bandits, f_player, GFA_DISP_HATE, 100, 0, 0);
    gfa_set_relation(&world, f_bandits, f_animals, GFA_DISP_PREY, 40, 0, 0);
    gfa_set_relation(&world, f_machines, f_bandits, GFA_DISP_CONTAIN, 95, 0, 0);
    gfa_set_relation(&world, f_player, f_machines, GFA_DISP_ALLY, 60, 0, 0);
    gfa_add_tag_rule(&world, t_machine, t_human, GFA_DISP_CONTAIN, 30, 0, 0);

    player = 1;
    bandit = 2;
    dog = 3;
    turret = 4;
    gfa_set_entity_faction(&world, player, f_player);
    gfa_add_entity_tag(&world, player, t_human);
    gfa_set_entity_stats(&world, player, 80, 100);
    gfa_set_entity_faction(&world, bandit, f_bandits);
    gfa_add_entity_tag(&world, bandit, t_human);
    gfa_set_entity_stats(&world, bandit, 70, 60);
    gfa_set_entity_faction(&world, dog, f_animals);
    gfa_set_entity_stats(&world, dog, 20, 20);
    gfa_set_entity_faction(&world, turret, f_machines);
    gfa_add_entity_tag(&world, turret, t_machine);
    gfa_set_entity_stats(&world, turret, 90, 0);

    ctx.stimulus = GFA_STIM_SIGHT;
    ctx.distance_fp = GFA_FP_FROM_INT(10);
    ctx.visible = 1;
    ctx.heard = 0;
    ctx.recent_damage = 0;
    ctx.target_threat = 0;
    ctx.target_morale = 0;
    ctx.mission_bias = 0;
    ctx.reserved0 = 0;
    ctx.reserved1 = 0;

    gfa_debug_print_matrix(&world);
    candidates[0] = player;
    candidates[1] = dog;
    candidates[2] = turret;
    candidates[3] = bandit;

    best = gfa_choose_best_target(&world, bandit, candidates, 4, &ctx, &d);
    printf("bandit best target=%d disp=%s score=%d\n", best,
        gfa_disposition_name(d.disposition), GFA_FP_TO_INT(d.score_fp));

    best = gfa_choose_best_target(&world, turret, candidates, 4, &ctx, &d);
    printf("turret best target=%d disp=%s score=%d\n", best,
        gfa_disposition_name(d.disposition), GFA_FP_TO_INT(d.score_fp));

    return 0;
}
