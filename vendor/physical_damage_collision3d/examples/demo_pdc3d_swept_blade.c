#include <stdio.h>
#include "pdc3d.h"

#define SOCKET_BASE 1
#define SOCKET_TIP  2

typedef struct demo_pose_s {
    int tick;
} demo_pose;

static hb3_v3 socket_pose(void *user, int actor_id, int socket_id)
{
    demo_pose *p;
    int x;
    (void)actor_id;
    p = (demo_pose *)user;
    x = -20 + p->tick * 10;
    if (socket_id == SOCKET_BASE) {
        return pdc3d_v3_from_ints(x, 30, 0);
    }
    return pdc3d_v3_from_ints(x + 18, 30, 0);
}

int main(void)
{
    pdc3d_world w;
    pdc3d_bridge b;
    pdc3d_attack_profile slash;
    pdc3d_event ev;
    demo_pose pose;
    int handle;
    int tick;

    pose.tick = 0;
    pdc3d_world_init(&w);
    pdc3d_bridge_init(&b);
    b.user = &pose;
    b.socket_pose = socket_pose;
    pdc3d_world_set_bridge(&w, &b);

    pdc3d_actor_add(&w, 10, 1);
    pdc3d_actor_add(&w, 20, 2);
    pdc3d_actor_add_hurt_capsule(&w, 20, 1,
        pdc3d_v3_from_ints(10, 0, 0),
        pdc3d_v3_from_ints(10, 55, 0),
        PDC3D_FX_FROM_INT(6), PDC3D_MAT_FLESH, 0);

    slash.attack_id = 200;
    slash.startup_frames = 0;
    slash.active_frames = 6;
    slash.recovery_frames = 2;
    slash.damage = 20;
    slash.stun_frames = 12;
    slash.hitstop_frames = 3;
    slash.flags = PDC3D_ATTACK_NO_SELF_HIT | PDC3D_ATTACK_NO_TEAM_HIT;
    slash.group_mask = 1;
    slash.hit_mask = HB3_ALL_MASK;
    pdc3d_attack_begin(&w, 10, &slash, &handle);

    for (tick = 0; tick < 7; ++tick) {
        pose.tick = tick;
        pdc3d_attack_set_hit_capsule_from_sockets(&w, handle, 0, 2,
            10, SOCKET_BASE, SOCKET_TIP, PDC3D_FX_FROM_INT(3),
            PDC3D_ATK_SLASH | PDC3D_ATK_PARRYABLE, 0);
        pdc3d_tick(&w);
    }

    while (pdc3d_poll_event(&w, &ev)) {
        if (ev.type == PDC3D_EVENT_DAMAGE) {
            printf("swept blade hit defender=%d final=%d point=(%d,%d,%d)\n",
                   ev.damage.defender_id, ev.damage.final_damage,
                   PDC3D_FX_TO_INT(ev.point.x),
                   PDC3D_FX_TO_INT(ev.point.y),
                   PDC3D_FX_TO_INT(ev.point.z));
        }
    }
    return 0;
}
