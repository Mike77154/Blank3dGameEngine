#include <stdio.h>
#include "pdc3d.h"

static void damage_cb(void *user, const pdc3d_damage_packet *p)
{
    (void)user;
    printf("callback damage: attacker=%d defender=%d base=%d final=%d weakspot=%d flags=%d\n",
           p->attacker_id, p->defender_id, p->base_damage,
           p->final_damage, p->weakspot_id, p->result_flags);
}

int main(void)
{
    pdc3d_world w;
    pdc3d_bridge b;
    pdc3d_attack_profile bite;
    pdc3d_event ev;
    int handle;
    int tick;

    pdc3d_world_init(&w);
    pdc3d_bridge_init(&b);
    b.damage_event = damage_cb;
    pdc3d_world_set_bridge(&w, &b);

    pdc3d_actor_add(&w, 1, 1);
    pdc3d_actor_add(&w, 2, 2);

    pdc3d_actor_add_hurt_capsule(&w, 2, 10,
        pdc3d_v3_from_ints(20, 0, 0),
        pdc3d_v3_from_ints(20, 40, 0),
        PDC3D_FX_FROM_INT(8), PDC3D_MAT_FLESH, 100);
    pdc3d_actor_add_hurt_sphere(&w, 2, 11,
        pdc3d_v3_from_ints(20, 48, 0),
        PDC3D_FX_FROM_INT(9), PDC3D_MAT_FLESH | PDC3D_MAT_HEAD, 101);
    pdc3d_actor_add_weakspot_sphere(&w, 2, 901,
        pdc3d_v3_from_ints(20, 48, 0),
        PDC3D_FX_FROM_INT(10), 3, 1, 2, W3D_WEAKSPOT_BREAKABLE);

    bite.attack_id = 3001;
    bite.startup_frames = 2;
    bite.active_frames = 4;
    bite.recovery_frames = 3;
    bite.damage = 12;
    bite.stun_frames = 10;
    bite.hitstop_frames = 2;
    bite.flags = PDC3D_ATTACK_NO_SELF_HIT | PDC3D_ATTACK_NO_TEAM_HIT;
    bite.group_mask = 1;
    bite.hit_mask = HB3_ALL_MASK;

    pdc3d_attack_begin(&w, 1, &bite, &handle);

    for (tick = 0; tick < 10; ++tick) {
        if (tick >= 2 && tick <= 5) {
            pdc3d_attack_set_hit_sphere(&w, handle, 0, 44,
                pdc3d_v3_from_ints(20, 48, 0),
                PDC3D_FX_FROM_INT(8),
                PDC3D_ATK_BITE | PDC3D_ATK_INFECT | PDC3D_ATK_GUARDABLE,
                77);
        }
        pdc3d_tick(&w);
    }

    while (pdc3d_poll_event(&w, &ev)) {
        if (ev.type == PDC3D_EVENT_DAMAGE) {
            printf("event damage: attacker=%d defender=%d final=%d weakspot=%d result_flags=%d\n",
                   ev.damage.attacker_id, ev.damage.defender_id,
                   ev.damage.final_damage, ev.damage.weakspot_id,
                   ev.damage.result_flags);
        } else if (ev.type == PDC3D_EVENT_WEAKSPOT) {
            printf("weakspot event: actor=%d weakspot=%d final=%d\n",
                   ev.actor_a, ev.actor_b, ev.value);
        }
    }

    return 0;
}
