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
    p->attack_id = 8001;
    p->hurtbox_index = 0;
    p->hitbox_index = 0;
    p->base_damage = damage;
    p->final_damage = damage;
    p->stun_frames = 10;
    p->hitstop_frames = 3;
    p->attack_flags = flags;
    p->material_flags = PDC3D_MAT_ARMOR;
    p->result_flags = 0;
    p->weakspot_id = -1;
    p->hit_user_tag = 0;
    p->hurt_user_tag = 0;
}

int main(void)
{
    pdc3d_world world;
    pdc3d_defense_profile profile;
    GSHL_LayerProfile shell_layer;
    pdc3d_damage_packet packet;
    pdc3d_defense_result result;
    pdc3d_defense_result counter_result;
    pdc3d_counter_request request;
    pdc3d_v3 source_dir;
    pdc3d_v3 facing_dir;
    int counter_status;

    pdc3d_world_init(&world);
    pdc3d_actor_add(&world, 30, 1);
    pdc3d_actor_add(&world, 40, 2);

    pdc3d_defense_profile_defaults(&profile);
    profile.enabled_flags = PDC3D_DEF_GUARD | PDC3D_DEF_SHELL | PDC3D_DEF_COUNTER;
    profile.guard.raise_frames = 1;
    profile.guard.chip_mul = 256;
    profile.guard.guard_damage_mul = 512;
    profile.counter.counter_kind = PDC3D_COUNTER_LIGHT;
    profile.counter.script_code = 7;

    pdc3d_defense_actor_enable(&world, 40, profile.enabled_flags, &profile);
    facing_dir = pdc3d_v3_make(PDC3D_FX_ONE, 0, 0);
    source_dir = pdc3d_v3_make(PDC3D_FX_ONE, 0, 0);
    pdc3d_defense_actor_set_facing(&world, 40, facing_dir);

    gshl_default_layer_profile(&shell_layer);
    shell_layer.max_hp = 12;
    shell_layer.absorb_mul = 1024;
    shell_layer.leak_mul = 0;
    shell_layer.recharge_per_frame = 0;
    pdc3d_defense_actor_set_shell_layer(&world, 40, 0, &shell_layer, 1);

    pdc3d_defense_actor_set_guard_input(&world, 40, 1);
    pdc3d_tick(&world);
    pdc3d_tick(&world);

    fill_packet(&packet, 30, 40, 40,
                PDC3D_ATK_BLUNT | PDC3D_ATK_GUARDABLE);
    pdc3d_resolve_incoming_attack(&world, &packet, source_dir, &result);

    request.actor_id = 40;
    request.target_id = 30;
    request.stamina_available = 999;
    request.posture_available = 999;
    request.target_rel_pos = source_dir;
    counter_status = pdc3d_counter_try(&world, &request, &counter_result);

    printf("pdc3d guard shell: final=%d chip=%d absorbed=%d guard_broken=%d shell_broken=%d counter=%d kind=%d script=%d\n",
           packet.final_damage,
           result.chip_damage,
           result.absorbed_damage,
           result.guard_broken,
           result.shell_broken,
           counter_status == PDC3D_OK ? 1 : 0,
           counter_result.counter_kind,
           counter_result.counter_script_code);
    return 0;
}
