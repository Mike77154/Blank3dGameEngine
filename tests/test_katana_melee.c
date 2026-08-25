#include <stdio.h>
#include <string.h>

#include "blank3d_katana_mesh.h"
#include "blank3d_katana_melee.h"

static g3d_vertex vertices[B3D_KATANA_RENDER_VERTEX_CAPACITY];
static g3d_index indices[B3D_KATANA_RENDER_INDEX_CAPACITY];

static int require_true(int condition, const char *message)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    Blank3DKatanaConfig config;
    Blank3DKatanaConfig loaded;
    Blank3DKatanaMelee melee;
    Blank3DKatanaHit hit;
    soq3d_pose root;
    g3d_mesh mesh;
    int frame;
    int hits;
    int saw_obb;
    int attack_index;
    int hitbox_index;
    int actor_index;
    int hurt_index;
    int saw_enemy_obb;
    int front_only_obb;
    int visual_front_only;
    const nm89_geometry_packet *packet;
    nm89_fx world_x;
    nm89_fx world_y;
    nm89_fx world_z;
    hb3_aabb attack_aabb;
    g3d_fx min_z;
    g3d_fx max_z;
    unsigned short vertex_index;

    blank3d_katana_config_defaults(&loaded);
    if (!require_true(blank3d_katana_config_load(&loaded,
            "config/melee/katana.ini"),
            "katana INI loads")) return 1;
    if (!require_true(loaded.active_start_frame == 7U &&
                      loaded.active_end_frame == 14U,
            "INI exposes frame-gated collision window")) return 1;
    if (!require_true(loaded.mount_lateral_cm == 0 &&
                      loaded.mount_height_cm == 105 &&
                      loaded.mount_forward_cm == 190,
            "INI places the complete swing pivot in front of player"))
        return 1;

    blank3d_katana_config_defaults(&config);
    config.debug_draw = 0;
    config.active_start_frame = 4U;
    config.active_end_frame = 11U;
    config.duration_frames = 20U;
    config.damage = 37;

    if (!require_true(blank3d_katana_mesh_build(&mesh, vertices,
            B3D_KATANA_RENDER_VERTEX_CAPACITY, indices,
            B3D_KATANA_RENDER_INDEX_CAPACITY, config.preset_index,
            config.mesh_scale_thickness_percent,
            config.mesh_scale_width_percent,
            config.mesh_scale_length_percent),
            "katana89 mesh converts into Giffany Shapes3D")) return 1;
    if (!require_true(mesh.vertex_count > 0U && mesh.index_count > 0U,
            "converted katana mesh is non-empty")) return 1;
    min_z = mesh.vertices[0].position.z;
    max_z = min_z;
    for (vertex_index = 1U; vertex_index < mesh.vertex_count;
         ++vertex_index) {
        if (mesh.vertices[vertex_index].position.z < min_z)
            min_z = mesh.vertices[vertex_index].position.z;
        if (mesh.vertices[vertex_index].position.z > max_z)
            max_z = mesh.vertices[vertex_index].position.z;
    }
    if (!require_true(max_z - min_z > G3D_FX_ONE,
            "katana visual mesh is deliberately long and large")) return 1;
    if (!require_true(blank3d_katana_melee_init(&melee, &config),
            "Mecanim/PDC katana subsystem initializes")) return 1;

    root = soq3d_pose_identity();
    root.position.x = (soq3d_fx)((loaded.mount_lateral_cm *
                                  SOQ3D_FX_ONE) / 100);
    root.position.y = (soq3d_fx)((loaded.mount_height_cm *
                                  SOQ3D_FX_ONE) / 100);
    root.position.z = (soq3d_fx)(-((loaded.mount_forward_cm *
                                    SOQ3D_FX_ONE) / 100));
    blank3d_katana_melee_set_root_pose(&melee, &root);
    /* A deliberately broad target around the origin catches the swept oriented blade box,
       while one-hit logging must still produce only one damage event. */
    if (!require_true(blank3d_katana_melee_set_target_q12(&melee,
            1000, 2, 1, 0, -4096, -8192, 16384, 8192),
            "enemy hurt volume registers")) return 1;
    saw_enemy_obb = 0;
    for (actor_index = 0; actor_index < HB3_MAX_ACTORS; ++actor_index) {
        if (!melee.collision.hurt.actors[actor_index].active ||
            melee.collision.hurt.actors[actor_index].id != 1000) continue;
        for (hurt_index = 0; hurt_index < HB3_MAX_HURTBOXES_PER_ACTOR;
             ++hurt_index) {
            if (melee.collision.hurt.actors[actor_index]
                    .hurtboxes[hurt_index].active &&
                melee.collision.hurt.actors[actor_index]
                    .hurtboxes[hurt_index].shape.type == HB3_SHAPE_OBB)
                saw_enemy_obb = 1;
        }
    }
    if (!require_true(saw_enemy_obb,
            "enemy vulnerability uses visible OBB box primitives")) return 1;
    if (!require_true(blank3d_katana_melee_trigger(&melee),
            "slash action triggers")) return 1;

    hits = 0;
    saw_obb = 0;
    front_only_obb = 1;
    visual_front_only = 1;
    for (frame = 0; frame < 28; ++frame) {
        blank3d_katana_melee_update(&melee, config.tick_ms);
        packet = blank3d_katana_melee_packet(&melee);
        if (packet) {
            for (vertex_index = 0U; vertex_index < mesh.vertex_count;
                 ++vertex_index) {
                nm89_matrix_transform_point(&packet->world,
                    mesh.vertices[vertex_index].position.x,
                    mesh.vertices[vertex_index].position.y,
                    mesh.vertices[vertex_index].position.z,
                    &world_x, &world_y, &world_z);
                (void)world_x;
                (void)world_y;
                if (world_z > (nm89_fx)(-(35 * NM89_FX_ONE) / 100))
                    visual_front_only = 0;
            }
        }
        for (attack_index = 0; attack_index < ML3_MAX_ATTACKS;
             ++attack_index) {
            if (!melee.collision.attacks[attack_index].active) continue;
            for (hitbox_index = 0;
                 hitbox_index < HIT3_MAX_HITBOXES_PER_ATTACK;
                 ++hitbox_index) {
                if (melee.collision.attacks[attack_index].hit
                        .hitboxes[hitbox_index].active &&
                    melee.collision.attacks[attack_index].hit
                        .hitboxes[hitbox_index].curr.type == HB3_SHAPE_OBB) {
                    saw_obb = 1;
                    if (hit3_hitbox_get_query_aabb(
                            &melee.collision.attacks[attack_index].hit
                                .hitboxes[hitbox_index],
                            &attack_aabb) != HB3_OK ||
                        attack_aabb.max_v.z >
                            (hb3_fx)(-(35 * HB3_FX_ONE) / 100))
                        front_only_obb = 0;
                }
            }
        }
        while (blank3d_katana_melee_poll_hit(&melee, &hit)) {
            if (!require_true(frame >= 3 && frame <= 13,
                    "damage is emitted only around configured active frames"))
                return 1;
            if (!require_true(hit.defender_id == 1000,
                    "damage targets the enemy actor")) return 1;
            if (!require_true(hit.damage == 37,
                    "configured damage crosses PDC3D")) return 1;
            ++hits;
        }
    }
    if (!require_true(saw_obb,
            "active katana collision is a true PDC3D OBB primitive")) return 1;
    if (!require_true(front_only_obb,
            "entire red attack OBB remains in front of player plane"))
        return 1;
    if (!require_true(visual_front_only,
            "entire visible katana remains in front of player plane"))
        return 1;
    if (!require_true(hits == 1,
            "one-hit-per-enemy suppresses repeated overlap damage")) return 1;
    if (!require_true(!blank3d_katana_melee_is_attacking(&melee),
            "slash returns to idle")) return 1;

    printf("PASS: giant katana front-only swing + red OBB + green enemy OBB hurtboxes\n");
    return 0;
}
