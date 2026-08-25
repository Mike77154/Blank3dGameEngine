#include "pdc3d.h"
#include <stdio.h>

static void fill_packet(pdc3d_damage_packet *p,
                        int attacker_id,
                        int defender_id,
                        int damage,
                        int flags)
{
    p->attacker_id = attacker_id;
    p->defender_id = defender_id;
    p->attack_id = 7001;
    p->hurtbox_index = 0;
    p->hitbox_index = 0;
    p->base_damage = damage;
    p->final_damage = damage;
    p->stun_frames = 8;
    p->hitstop_frames = 2;
    p->attack_flags = flags;
    p->material_flags = PDC3D_MAT_FLESH;
    p->result_flags = 0;
    p->weakspot_id = -1;
    p->hit_user_tag = 0;
    p->hurt_user_tag = 0;
}

int main(void)
{
    pdc3d_world world;
    pdc3d_defense_profile profile;
    pdc3d_damage_packet packet;
    pdc3d_defense_result result;
    pdc3d_defense_result counter_result;
    pdc3d_counter_request request;
    pdc3d_v3 source_dir;
    pdc3d_v3 facing_dir;
    int counter_status;

    pdc3d_world_init(&world);
    pdc3d_actor_add(&world, 10, 1);
    pdc3d_actor_add(&world, 20, 2);

    pdc3d_defense_profile_defaults(&profile);
    profile.enabled_flags = PDC3D_DEF_PARRY | PDC3D_DEF_COUNTER;
    profile.parry.startup_frames = 0;
    profile.parry.perfect_frames = 4;
    profile.parry.normal_frames = 4;
    profile.parry.late_frames = 2;
    profile.counter.valid_frames = 20;
    profile.counter.token_life_frames = 30;
    profile.counter.counter_kind = PDC3D_COUNTER_HEAVY;
    profile.counter.script_code = 42;

    pdc3d_defense_actor_enable(&world, 20, profile.enabled_flags, &profile);
    facing_dir = pdc3d_v3_make(PDC3D_FX_ONE, 0, 0);
    source_dir = pdc3d_v3_make(PDC3D_FX_ONE, 0, 0);
    pdc3d_defense_actor_set_facing(&world, 20, facing_dir);

    pdc3d_defense_actor_press_parry(&world, 20);
    pdc3d_tick(&world);

    fill_packet(&packet, 10, 20, 36,
                PDC3D_ATK_SLASH | PDC3D_ATK_GUARDABLE |
                PDC3D_ATK_PARRYABLE);
    pdc3d_resolve_incoming_attack(&world, &packet, source_dir, &result);

    request.actor_id = 20;
    request.target_id = 10;
    request.stamina_available = 999;
    request.posture_available = 999;
    request.target_rel_pos = source_dir;
    counter_status = pdc3d_counter_try(&world, &request, &counter_result);

    printf("pdc3d defense pipeline: final=%d parry=%d flags=%u tokens=%d counter=%d kind=%d script=%d\n",
           packet.final_damage,
           result.parry_result,
           result.defense_flags,
           pdc3d_counter_count_tokens(&world, 20),
           counter_status == PDC3D_OK ? 1 : 0,
           counter_result.counter_kind,
           counter_result.counter_script_code);
    return 0;
}
