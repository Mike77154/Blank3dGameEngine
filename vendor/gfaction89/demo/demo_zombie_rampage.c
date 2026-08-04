#include "gfaction89.h"
#include <stdio.h>

static void on_faction_changed(GFA_World *world, int entity_id, int old_faction, int new_faction, void *user)
{
    (void)user;
    printf("HOOK faction_changed entity=%d %s -> %s\n", entity_id,
        gfa_faction_name(world, old_faction), gfa_faction_name(world, new_faction));
}

static void on_trigger(GFA_World *world, const GFA_Event *event_data, const GFA_Trigger *trigger_data, void *user)
{
    (void)world;
    (void)user;
    printf("HOOK trigger=%s event=%d src=%d dst=%d\n", trigger_data->name,
        event_data->event_type, event_data->source_entity, event_data->target_entity);
}

int main(void)
{
    GFA_World world;
    GFA_Hooks hooks;
    GFA_Context ctx;
    GFA_Decision d;
    GFA_Event event_data;
    int candidates[3];
    int best;

    gfa_init(&world);
    if (!gfa_load_ini_file(&world, "demo/rampage.ini")) {
        if (!gfa_load_ini_file(&world, "rampage.ini")) {
            printf("could not load demo/rampage.ini\n");
            return 1;
        }
    }

    hooks.on_faction_changed = on_faction_changed;
    hooks.on_decision = 0;
    hooks.on_trigger = on_trigger;
    hooks.user = 0;
    gfa_add_hooks(&world, &hooks);

    ctx.stimulus = GFA_STIM_SIGHT;
    ctx.distance_fp = GFA_FP_FROM_INT(5);
    ctx.visible = 1;
    ctx.heard = 0;
    ctx.recent_damage = 0;
    ctx.target_threat = 0;
    ctx.target_morale = 0;
    ctx.mission_bias = 0;
    ctx.reserved0 = 0;
    ctx.reserved1 = 0;

    printf("== Initial entities ==\n");
    gfa_debug_print_entity(&world, 1);
    gfa_debug_print_entity(&world, 10);
    gfa_debug_print_entity(&world, 20);
    gfa_debug_print_entity(&world, 30);

    printf("\n== Decisions ==\n");
    d = gfa_eval(&world, 30, 1, &ctx);
    gfa_debug_print_decision(&world, 30, 1, &d);
    d = gfa_eval(&world, 20, 30, &ctx);
    gfa_debug_print_decision(&world, 20, 30, &d);
    d = gfa_eval(&world, 10, 20, &ctx);
    gfa_debug_print_decision(&world, 10, 20, &d);

    candidates[0] = 1;
    candidates[1] = 10;
    candidates[2] = 20;
    best = gfa_choose_best_target(&world, 30, candidates, 3, &ctx, &d);
    printf("\nZombie best attack target = %d disposition=%s score=%d\n",
        best, gfa_disposition_name(d.disposition), GFA_FP_TO_INT(d.score_fp));

    printf("\n== Death event: zombie 30 kills civilian 20 ==\n");
    event_data.event_type = GFA_EVENT_DEATH;
    event_data.source_entity = 30;
    event_data.target_entity = 20;
    event_data.amount = 999;
    event_data.user_code = 0;
    gfa_fire_event(&world, &event_data);
    gfa_debug_print_entity(&world, 20);

    printf("\n== After conversion ==\n");
    d = gfa_eval(&world, 20, 1, &ctx);
    gfa_debug_print_decision(&world, 20, 1, &d);
    d = gfa_eval(&world, 30, 20, &ctx);
    gfa_debug_print_decision(&world, 30, 20, &d);

    return 0;
}
