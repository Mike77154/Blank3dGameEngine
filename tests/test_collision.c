#include <stdio.h>
#include <string.h>
#include "blank3d_collision.h"

int main(void)
{
    Blank3DCollision collision;
    Blank3DCollisionHit hit;
    GWP89_Vec3 start;
    GWP89_Vec3 delta;
    GWP89_ProviderPacket packet;
    int result;

    blank3d_collision_init(&collision);
    blank3d_collision_set_enemy(&collision, 0, 1,
        0, 0, 10L * GWP89_FIX_ONE);
    blank3d_collision_step(&collision);

    start.x = 0; start.y = GWP89_FIX_ONE; start.z = 0;
    delta.x = 0; delta.y = 0; delta.z = 20L * GWP89_FIX_ONE;
    if (!blank3d_collision_sweep_bullet(&collision, &start, &delta,
            GWP89_FIX_ONE / 10L, &hit)) return 1;
    if (!hit.hit || hit.enemy_index != 0) return 2;

    memset(&packet, 0, sizeof(packet));
    packet.phase = GWP89_PHASE_PRE;
    packet.operation = GWP89_OP_RAYCAST;
    packet.vec_a = start;
    packet.vec_b.x = 0;
    packet.vec_b.y = 0;
    packet.vec_b.z = GWP89_FIX_ONE;
    packet.fx_value = 20L * GWP89_FIX_ONE;
    result = blank3d_collision_weapon_provider(&collision, &packet);
    if (result != GWP89_PROVIDER_HANDLED) return 3;
    if (!packet.hit.hit || packet.hit.actor_id != 100) return 4;

    /* Reproduce the gunner self-hit: a bullet starts close to enemy 0.
       The ordinary mask sees the capsule, but NPC world-only filtering must
       ignore every enemy body and preserve the shooter. */
    blank3d_collision_set_enemy(&collision, 0, 1,
        0, 0, 0);
    blank3d_collision_step(&collision);
    start.x = 0;
    start.y = (12L * GWP89_FIX_ONE) / 10L;
    start.z = (7L * GWP89_FIX_ONE) / 10L;
    delta.x = 8L * GWP89_FIX_ONE;
    delta.y = 0;
    delta.z = 0;
    if (!blank3d_collision_sweep_bullet_mask(&collision, &start, &delta,
            GWP89_FIX_ONE / 10L, B3D_COLLISION_LAYER_ENEMY, &hit)) return 5;
    if (hit.enemy_index != 0) return 6;
    if (blank3d_collision_sweep_bullet_mask(&collision, &start, &delta,
            GWP89_FIX_ONE / 10L, B3D_COLLISION_LAYER_WORLD, &hit)) return 7;

    puts("Blank3D collision provider test: OK");
    return 0;
}
