#include "pdc3d.h"
#include <stdio.h>
#include <string.h>

static const pdc3d_fd_hitdef g_slash_hit = {
    FD3D_ATTR_STRIKE | FD3D_ATTR_MID,
    FD3D_LANE_ALL,
    25,
    4,
    17,
    9,
    3,
    2,
    0,
    0,
    0,
    1001u,
    0x0001u,
    0x0002u
};

static const pdc3d_fd_frame g_attacker_frames[] = {
    { 3u, 0u, 0ul, 0u, FD3D_LANE_ALL, 0, 0u, 0, 0u, 0, 0u, 0, 0u, &g_slash_hit }
};

static const pdc3d_fd_action g_slash_action = {
    77u,
    "fd_slash_active",
    g_attacker_frames,
    1u,
    0u,
    1u
};

static const pdc3d_fd_frame g_iframe_frames[] = {
    { 4u, 0u, FD3D_FLAG_STRIKE_INVULN, 0u, FD3D_LANE_ALL, 0, 0u, 0, 0u, 0, 0u, 0, 0u, 0 }
};

static const pdc3d_fd_action g_iframe_action = {
    88u,
    "fd_strike_iframes",
    g_iframe_frames,
    1u,
    0u,
    1u
};

static const pdc3d_fd_frame g_neutral_frames[] = {
    { 8u, 0u, 0ul, 0u, FD3D_LANE_ALL, 0, 0u, 0, 0u, 0, 0u, 0, 0u, 0 }
};

static const pdc3d_fd_action g_neutral_action = {
    89u,
    "fd_neutral",
    g_neutral_frames,
    1u,
    1u,
    1u
};

static void make_packet(pdc3d_damage_packet *p)
{
    memset(p, 0, sizeof(*p));
    p->attacker_id = 1;
    p->defender_id = 2;
    p->attack_id = 77;
    p->base_damage = 5;
    p->final_damage = 5;
    p->attack_flags = PDC3D_ATK_SLASH | PDC3D_ATK_GUARDABLE;
    p->material_flags = PDC3D_MAT_FLESH;
    p->weakspot_id = -1;
}

int main(void)
{
    pdc3d_world w;
    pdc3d_damage_packet p;
    pdc3d_framedata_result fd;

    pdc3d_world_init(&w);
    pdc3d_fd_actor_enable(&w, 1, 1, 1000);
    pdc3d_fd_actor_enable(&w, 2, 2, 1000);
    pdc3d_fd_actor_set_pose(&w, 1, pdc3d_v3_from_ints(0, 0, 0),
                             PDC3D_FD_FACE_Z_POS, PDC3D_FD_LANE_CENTER);
    pdc3d_fd_actor_set_pose(&w, 2, pdc3d_v3_from_ints(0, 0, 8),
                             PDC3D_FD_FACE_Z_NEG, PDC3D_FD_LANE_CENTER);
    pdc3d_fd_actor_set_action(&w, 1, &g_slash_action);
    pdc3d_fd_actor_set_action(&w, 2, &g_iframe_action);

    make_packet(&p);
    pdc3d_fd_resolve_actor_damage(&w, 1, 2, -1, &p, &fd);
    printf("iframes: can_hit=%d final=%d flags=%lu\n",
           fd.can_hit, p.final_damage, (unsigned long)fd.result_flags);

    pdc3d_fd_actor_set_action(&w, 2, &g_neutral_action);
    make_packet(&p);
    pdc3d_fd_resolve_actor_damage(&w, 1, 2, -1, &p, &fd);
    printf("active: can_hit=%d final=%d hitstop=%d hitstun=%d\n",
           fd.can_hit, p.final_damage, fd.hitstop_frames, fd.hitstun_frames);

    pdc3d_fd_resolve_actor_damage(&w, 1, 2, 0, &p, &fd);
    printf("hit apply: adv_hit=%d contact=%u\n",
           fd.frame_advantage_hit,
           (unsigned int)((fd.result_flags & PDC3D_FDRES_CONTACT_APPLIED) != 0u));

    pdc3d_fd_resolve_actor_damage(&w, 1, 2, 1, &p, &fd);
    printf("block apply: adv_block=%d blockstun=%d blockstop=%d\n",
           fd.frame_advantage_block, fd.blockstun_frames, fd.blockstop_frames);
    return 0;
}
