#include <stdio.h>
#include <string.h>

#include "blank3d_faction.h"

static void make_context(GFA_Context *context, int distance)
{
    memset(context, 0, sizeof(*context));
    context->distance_fp = GFA_FP_FROM_INT(distance);
    context->target_threat = 60;
}

int main(void)
{
    Blank3DFactionSystem system;
    Blank3DFactionCandidate candidates[4];
    GFA_Decision decision;
    GFA_Context context;
    int target;

    if (!blank3d_faction_init(&system, "config/factions/gameplay.ini")) {
        puts(blank3d_faction_status(&system));
        return 1;
    }
    if (!blank3d_faction_register_actor(&system, 1, "player", "survivors",
            "hero", "human,organic,armed,player,targetable",
            85, 100, 1, 1)) return 2;
    if (!blank3d_faction_register_actor(&system, 1000, "hostile", "monsters",
            "attacker", "organic,enemy,targetable",
            65, 50, 1, 1)) return 3;
    if (!blank3d_faction_register_actor(&system, 1001, "allies", "survivors",
            "armed_ally", "human,organic,armed,ally,targetable",
            70, 90, 1, 1)) return 4;
    if (!blank3d_faction_register_actor(&system, 1002, "hostile", "monsters",
            "attacker", "organic,enemy,targetable",
            65, 50, 1, 1)) return 5;
    if (!blank3d_faction_register_actor(&system, 1003, "neutral", "civilians",
            "noncombatant", "human,organic,targetable",
            10, 30, 1, 1)) return 14;

    make_context(&context, 5);
    if (blank3d_faction_can_attack(&system, 1001, 1, &context)) return 6;
    if (!blank3d_faction_are_allies(&system, 1, 1001)) return 7;
    if (!blank3d_faction_can_attack(&system, 1001, 1000, &context)) return 8;
    if (blank3d_faction_can_attack(&system, 1000, 1002, &context)) return 9;
    if (!blank3d_faction_can_attack(&system, 1000, 1, &context)) return 10;
    if (!blank3d_faction_can_attack(&system, 1000, 1001, &context)) return 11;
    if (blank3d_faction_can_attack(&system, 1001, 1003, &context)) return 15;
    if (blank3d_faction_can_attack(&system, 1003, 1, &context)) return 16;

    candidates[0].entity_id = 1;
    make_context(&candidates[0].context, 12);
    candidates[1].entity_id = 1000;
    make_context(&candidates[1].context, 4);
    candidates[2].entity_id = 1002;
    make_context(&candidates[2].context, 8);
    candidates[3].entity_id = 1003;
    make_context(&candidates[3].context, 1);
    target = blank3d_faction_choose_attack_target(&system, 1001,
                                                   candidates, 4, &decision);
    if (target != 1000 || !decision.can_attack) return 12;

    candidates[0].entity_id = 1;
    make_context(&candidates[0].context, 10);
    candidates[1].entity_id = 1001;
    make_context(&candidates[1].context, 3);
    candidates[1].context.recent_damage = 1;
    candidates[2].entity_id = 1002;
    make_context(&candidates[2].context, 1);
    candidates[3].entity_id = 1003;
    make_context(&candidates[3].context, 1);
    target = blank3d_faction_choose_attack_target(&system, 1000,
                                                   candidates, 4, &decision);
    if (target != 1001 || !decision.can_attack) return 13;

    /* A hostile must be able to prefer a nearer armed ally over the player.
       This catches the old player-first enumeration/threat bias. */
    candidates[0].entity_id = 1;
    make_context(&candidates[0].context, 18);
    candidates[1].entity_id = 1001;
    make_context(&candidates[1].context, 11);
    candidates[2].entity_id = 1002;
    make_context(&candidates[2].context, 2);
    candidates[3].entity_id = 1003;
    make_context(&candidates[3].context, 1);
    target = blank3d_faction_choose_attack_target_stable(&system, 1000,
        candidates, 4, GFA_INVALID_ID, 1, 3, &decision);
    if (target != 1001 || !decision.can_attack) return 17;

    /* A small current-target bonus prevents frame-to-frame oscillation, but
       recent damage from the ally must override it immediately. */
    target = blank3d_faction_choose_attack_target_stable(&system, 1000,
        candidates, 4, 1, 1, 3, &decision);
    if (target != 1) return 18;
    candidates[1].context.recent_damage = 1;
    target = blank3d_faction_choose_attack_target_stable(&system, 1000,
        candidates, 4, 1, 1, 3, &decision);
    if (target != 1001) return 19;

    puts("PASS: hostiles recognize player and armed allies, with distance, stickiness and damage retargeting");
    return 0;
}
