#include <string.h>

#include "blank3d_projectile_mesh.h"
#include "gbulletmesh89.h"
#include "rocketmeshes.h"
#include "ghandgrenade3d89.h"

#define B3D_GBM_SCRATCH_VERTICES 512
#define B3D_GBM_SCRATCH_TRIANGLES 1024

static GBM_Vertex b3d_gbm_vertices[B3D_GBM_SCRATCH_VERTICES];
static GBM_Tri b3d_gbm_triangles[B3D_GBM_SCRATCH_TRIANGLES];
static GBM_Mesh b3d_gbm_mesh;

static g3d_color b3d_rm_material_color(unsigned char material)
{
    switch (material) {
    case RM_MAT_NOSE:   return g3d_color_rgba(96U, 106U, 94U, 255U);
    case RM_MAT_BAND:   return g3d_color_rgba(183U, 145U, 49U, 255U);
    case RM_MAT_FIN:    return g3d_color_rgba(72U, 82U, 72U, 255U);
    case RM_MAT_MOTOR:  return g3d_color_rgba(58U, 61U, 64U, 255U);
    case RM_MAT_TIP:    return g3d_color_rgba(150U, 62U, 42U, 255U);
    case RM_MAT_DETAIL: return g3d_color_rgba(45U, 48U, 43U, 255U);
    case RM_MAT_BRASS:  return g3d_color_rgba(199U, 145U, 52U, 255U);
    case RM_MAT_EMPTY:  return g3d_color_rgba(32U, 32U, 34U, 255U);
    case RM_MAT_SMOKE:  return g3d_color_rgba(155U, 155U, 155U, 210U);
    default:            return g3d_color_rgba(103U, 116U, 88U, 255U);
    }
}

static void b3d_zero_vertex(g3d_vertex *vertex)
{
    if (!vertex) return;
    memset(vertex, 0, sizeof(*vertex));
    vertex->color = g3d_color_rgba(255U, 255U, 255U, 255U);
}

static void b3d_compute_mesh_normals(g3d_mesh *mesh)
{
    unsigned short i;
    unsigned short ia;
    unsigned short ib;
    unsigned short ic;
    g3d_vec3 ab;
    g3d_vec3 ac;
    g3d_vec3 normal;

    if (!mesh || !mesh->vertices || !mesh->indices) return;
    for (i = 0U; i < mesh->vertex_count; ++i)
        mesh->vertices[i].normal = g3d_vec3_make(0L, 0L, 0L);

    i = 0U;
    while ((unsigned long)i + 2UL < (unsigned long)mesh->index_count) {
        ia = mesh->indices[i];
        ib = mesh->indices[(unsigned short)(i + 1U)];
        ic = mesh->indices[(unsigned short)(i + 2U)];
        if (ia < mesh->vertex_count && ib < mesh->vertex_count &&
            ic < mesh->vertex_count) {
            ab = g3d_vec3_sub(mesh->vertices[ib].position,
                              mesh->vertices[ia].position);
            ac = g3d_vec3_sub(mesh->vertices[ic].position,
                              mesh->vertices[ia].position);
            normal = g3d_vec3_cross(ab, ac);
            mesh->vertices[ia].normal =
                g3d_vec3_add(mesh->vertices[ia].normal, normal);
            mesh->vertices[ib].normal =
                g3d_vec3_add(mesh->vertices[ib].normal, normal);
            mesh->vertices[ic].normal =
                g3d_vec3_add(mesh->vertices[ic].normal, normal);
        }
        i = (unsigned short)(i + 3U);
    }

    for (i = 0U; i < mesh->vertex_count; ++i) {
        if (g3d_vec3_length_sq(mesh->vertices[i].normal) <= G3D_FX_EPSILON)
            mesh->vertices[i].normal = g3d_vec3_make(0L, 0L, G3D_FX_ONE);
        else
            mesh->vertices[i].normal =
                g3d_vec3_normalize(mesh->vertices[i].normal);
    }
}

static int b3d_convert_gbm(g3d_mesh *destination,
                           g3d_vertex *vertices,
                           unsigned short vertex_capacity,
                           g3d_index *indices,
                           unsigned short index_capacity,
                           int ammo_type,
                           int single_shotgun_pellet,
                           int gatling_variant)
{
    unsigned short i;
    unsigned short vertex_count;
    unsigned short triangle_count;
    int result;

    gbm_mesh_init(&b3d_gbm_mesh,
                  b3d_gbm_vertices, B3D_GBM_SCRATCH_VERTICES,
                  b3d_gbm_triangles, B3D_GBM_SCRATCH_TRIANGLES);
    if (single_shotgun_pellet)
        result = gbm_build_shotgun_pellet(&b3d_gbm_mesh);
    else
        result = gbm_build_projectile(&b3d_gbm_mesh, ammo_type);
    if (result != GBM_OK) return 0;

    vertex_count = b3d_gbm_mesh.v_count;
    triangle_count = b3d_gbm_mesh.t_count;
    if (vertex_count > vertex_capacity) return 0;
    if ((unsigned long)triangle_count * 3UL > (unsigned long)index_capacity)
        return 0;
    if (g3d_mesh_init(destination, vertices, vertex_capacity,
                      indices, index_capacity) != G3D_OK) return 0;

    for (i = 0U; i < vertex_count; ++i) {
        unsigned int r;
        unsigned int g;
        unsigned int b;
        b3d_zero_vertex(&vertices[i]);
        vertices[i].position.x = (g3d_fx)b3d_gbm_mesh.v[i].x;
        vertices[i].position.y = (g3d_fx)b3d_gbm_mesh.v[i].y;
        vertices[i].position.z = (g3d_fx)b3d_gbm_mesh.v[i].z;
        r = b3d_gbm_mesh.v[i].r;
        g = b3d_gbm_mesh.v[i].g;
        b = b3d_gbm_mesh.v[i].b;
        if (gatling_variant) {
            r = r + 28U; if (r > 255U) r = 255U;
            g = g + 18U; if (g > 255U) g = 255U;
        }
        vertices[i].color = g3d_color_rgba((unsigned char)r,
                                             (unsigned char)g,
                                             (unsigned char)b,
                                             b3d_gbm_mesh.v[i].a);
    }
    for (i = 0U; i < triangle_count; ++i) {
        indices[(unsigned short)(i * 3U)] = b3d_gbm_mesh.t[i].a;
        indices[(unsigned short)(i * 3U + 1U)] = b3d_gbm_mesh.t[i].b;
        indices[(unsigned short)(i * 3U + 2U)] = b3d_gbm_mesh.t[i].c;
    }
    destination->vertex_count = vertex_count;
    destination->index_count = (unsigned short)(triangle_count * 3U);
    destination->primitive = G3D_PRIMITIVE_TRIANGLES;
    destination->error = G3D_OK;
    b3d_compute_mesh_normals(destination);
    return 1;
}

static int b3d_convert_gbm_shell(g3d_mesh *destination,
                                 g3d_vertex *vertices,
                                 unsigned short vertex_capacity,
                                 g3d_index *indices,
                                 unsigned short index_capacity,
                                 int ammo_type)
{
    unsigned short i;
    unsigned short vertex_count;
    unsigned short triangle_count;
    int result;

    gbm_mesh_init(&b3d_gbm_mesh,
                  b3d_gbm_vertices, B3D_GBM_SCRATCH_VERTICES,
                  b3d_gbm_triangles, B3D_GBM_SCRATCH_TRIANGLES);
    result = gbm_build_shell(&b3d_gbm_mesh, ammo_type);
    if (result != GBM_OK) return 0;

    vertex_count = b3d_gbm_mesh.v_count;
    triangle_count = b3d_gbm_mesh.t_count;
    if (vertex_count > vertex_capacity) return 0;
    if ((unsigned long)triangle_count * 3UL > (unsigned long)index_capacity)
        return 0;
    if (g3d_mesh_init(destination, vertices, vertex_capacity,
                      indices, index_capacity) != G3D_OK) return 0;

    for (i = 0U; i < vertex_count; ++i) {
        b3d_zero_vertex(&vertices[i]);
        vertices[i].position.x = (g3d_fx)b3d_gbm_mesh.v[i].x;
        vertices[i].position.y = (g3d_fx)b3d_gbm_mesh.v[i].y;
        vertices[i].position.z = (g3d_fx)b3d_gbm_mesh.v[i].z;
        vertices[i].color = g3d_color_rgba(b3d_gbm_mesh.v[i].r,
                                           b3d_gbm_mesh.v[i].g,
                                           b3d_gbm_mesh.v[i].b,
                                           b3d_gbm_mesh.v[i].a);
    }
    for (i = 0U; i < triangle_count; ++i) {
        indices[(unsigned short)(i * 3U)] = b3d_gbm_mesh.t[i].a;
        indices[(unsigned short)(i * 3U + 1U)] = b3d_gbm_mesh.t[i].b;
        indices[(unsigned short)(i * 3U + 2U)] = b3d_gbm_mesh.t[i].c;
    }
    destination->vertex_count = vertex_count;
    destination->index_count = (unsigned short)(triangle_count * 3U);
    destination->primitive = G3D_PRIMITIVE_TRIANGLES;
    destination->error = G3D_OK;
    b3d_compute_mesh_normals(destination);
    return 1;
}

static int b3d_convert_rocket(g3d_mesh *destination,
                              g3d_vertex *vertices,
                              unsigned short vertex_capacity,
                              g3d_index *indices,
                              unsigned short index_capacity,
                              int rocket_mesh_id)
{
    const RM_Mesh *source;
    unsigned short i;
    g3d_color color;

    source = rm_get_mesh(rocket_mesh_id);
    if (!source) return 0;
    if (source->vcount > vertex_capacity) return 0;
    if ((unsigned long)source->tcount * 3UL > (unsigned long)index_capacity)
        return 0;
    if (g3d_mesh_init(destination, vertices, vertex_capacity,
                      indices, index_capacity) != G3D_OK) return 0;

    /* rocketmeshes uses local +X as the long axis. Blank3D standardizes all
       projectile providers to local +Z, so the renderer can orient one way. */
    for (i = 0U; i < source->vcount; ++i) {
        b3d_zero_vertex(&vertices[i]);
        vertices[i].position.x = (g3d_fx)source->v[i].y * 256L;
        vertices[i].position.y = (g3d_fx)source->v[i].z * 256L;
        vertices[i].position.z = (g3d_fx)source->v[i].x * 256L;
        vertices[i].color = g3d_color_rgba(103U, 116U, 88U, 255U);
    }
    for (i = 0U; i < source->tcount; ++i) {
        unsigned short base;
        base = (unsigned short)(i * 3U);
        indices[base] = source->t[i].a;
        indices[(unsigned short)(base + 1U)] = source->t[i].b;
        indices[(unsigned short)(base + 2U)] = source->t[i].c;
        color = b3d_rm_material_color(source->t[i].mat);
        if (source->t[i].a < source->vcount)
            vertices[source->t[i].a].color = color;
        if (source->t[i].b < source->vcount)
            vertices[source->t[i].b].color = color;
        if (source->t[i].c < source->vcount)
            vertices[source->t[i].c].color = color;
    }
    destination->vertex_count = source->vcount;
    destination->index_count = (unsigned short)(source->tcount * 3U);
    destination->primitive = G3D_PRIMITIVE_TRIANGLES;
    destination->error = G3D_OK;
    b3d_compute_mesh_normals(destination);
    return 1;
}


#define B3D_GHG_VERTEX_CAPACITY 512
#define B3D_GHG_TRIANGLE_CAPACITY 768
static GHG3D_Vertex b3d_ghg_vertices[B3D_GHG_VERTEX_CAPACITY];
static GHG3D_Triangle b3d_ghg_triangles[B3D_GHG_TRIANGLE_CAPACITY];

static int b3d_convert_hand_grenade(g3d_mesh *destination,
                                    g3d_vertex *vertices,
                                    unsigned short vertex_capacity,
                                    g3d_index *indices,
                                    unsigned short index_capacity,
                                    int preset)
{
    GHG3D_Desc desc;
    GHG3D_Result result;
    unsigned long i;
    unsigned long index_count;
    g3d_color color;
    ghg3d_desc_default(&desc, preset);
    desc.lod = GHG3D_LOD_GAME;
    desc.parts_mask = GHG3D_PARTMASK_ALL;
    if (ghg3d_build(&desc, b3d_ghg_vertices, B3D_GHG_VERTEX_CAPACITY,
                    b3d_ghg_triangles, B3D_GHG_TRIANGLE_CAPACITY,
                    &result) != GHG3D_OK) return 0;
    index_count = result.triangle_count * 3UL;
    if (result.vertex_count > vertex_capacity || index_count > index_capacity)
        return 0;
    if (g3d_mesh_init(destination, vertices, vertex_capacity,
                      indices, index_capacity) != G3D_OK) return 0;
    for (i = 0UL; i < result.vertex_count; ++i) {
        b3d_zero_vertex(&vertices[i]);
        /* Provider is Q24.8 millimetres. Convert to Q16 world units and
           normalize the grenade height to roughly one world unit. */
        vertices[i].position.x = (g3d_fx)(b3d_ghg_vertices[i].x * 5L);
        vertices[i].position.y = (g3d_fx)(b3d_ghg_vertices[i].z * 5L);
        vertices[i].position.z = (g3d_fx)(b3d_ghg_vertices[i].y * 5L);
        if (b3d_ghg_vertices[i].part == GHG3D_PART_SAFETY)
            color = g3d_color_rgba(176U, 178U, 170U, 255U);
        else if (b3d_ghg_vertices[i].part == GHG3D_PART_HOLDER)
            color = g3d_color_rgba(67U, 72U, 61U, 255U);
        else
            color = g3d_color_rgba(82U, 101U, 63U, 255U);
        vertices[i].color = color;
    }
    for (i = 0UL; i < result.triangle_count; ++i) {
        indices[i * 3UL] = b3d_ghg_triangles[i].a;
        indices[i * 3UL + 1UL] = b3d_ghg_triangles[i].b;
        indices[i * 3UL + 2UL] = b3d_ghg_triangles[i].c;
    }
    destination->vertex_count = (unsigned short)result.vertex_count;
    destination->index_count = (unsigned short)index_count;
    destination->primitive = G3D_PRIMITIVE_TRIANGLES;
    destination->error = G3D_OK;
    b3d_compute_mesh_normals(destination);
    return 1;
}

static int b3d_make_energy_bolt(g3d_mesh *mesh,
                                g3d_vertex *vertices,
                                unsigned short vertex_capacity,
                                g3d_index *indices,
                                unsigned short index_capacity,
                                g3d_fx radius, g3d_fx length,
                                g3d_color color)
{
    unsigned short i;
    g3d_fx old_y;
    if (g3d_mesh_init(mesh, vertices, vertex_capacity,
                      indices, index_capacity) != G3D_OK) return 0;
    if (g3d_make_cylinder(mesh, radius, length, 16U, color) != G3D_OK) return 0;
    for (i = 0U; i < mesh->vertex_count; ++i) {
        old_y = mesh->vertices[i].position.y;
        mesh->vertices[i].position.y = mesh->vertices[i].position.z;
        mesh->vertices[i].position.z = old_y;
        old_y = mesh->vertices[i].normal.y;
        mesh->vertices[i].normal.y = mesh->vertices[i].normal.z;
        mesh->vertices[i].normal.z = old_y;
    }
    return 1;
}

int blank3d_projectile_mesh_build(int mesh_id,
                                  g3d_mesh *mesh,
                                  g3d_vertex *vertices,
                                  unsigned short vertex_capacity,
                                  g3d_index *indices,
                                  unsigned short index_capacity)
{
    if (!mesh || !vertices || !indices) return 0;
    switch (mesh_id) {
    case 1:
        return b3d_convert_gbm(mesh, vertices, vertex_capacity,
                               indices, index_capacity,
                               GBM_AMMO_PISTOL, 0, 0);
    case 2:
        return b3d_convert_gbm(mesh, vertices, vertex_capacity,
                               indices, index_capacity,
                               GBM_AMMO_MACHINEGUN, 0, 0);
    case 3:
        return b3d_convert_gbm(mesh, vertices, vertex_capacity,
                               indices, index_capacity,
                               GBM_AMMO_SHOTGUN, 1, 0);
    case 4:
        return b3d_convert_gbm(mesh, vertices, vertex_capacity,
                               indices, index_capacity,
                               GBM_AMMO_MAGNUM, 0, 0);
    case 5:
        return b3d_convert_gbm(mesh, vertices, vertex_capacity,
                               indices, index_capacity,
                               GBM_AMMO_SNIPER, 0, 0);
    case 6:
        return b3d_convert_rocket(mesh, vertices, vertex_capacity,
                                  indices, index_capacity,
                                  RMESH_GRENADE40_LV);
    case 7:
        return b3d_convert_rocket(mesh, vertices, vertex_capacity,
                                  indices, index_capacity,
                                  RMESH_SURVIVAL_RPG7_CONE);
    case 8:
        return b3d_convert_gbm(mesh, vertices, vertex_capacity,
                               indices, index_capacity,
                               GBM_AMMO_MACHINEGUN, 0, 1);
    case 9:
        if (g3d_mesh_init(mesh, vertices, vertex_capacity,
                          indices, index_capacity) != G3D_OK) return 0;
        return g3d_make_uv_sphere(mesh, g3d_fx_from_ratio(9L, 20L),
                                  12U, 8U,
                                  g3d_color_rgba(102U, 94U, 84U, 255U))
               == G3D_OK;
    case 10:
        return b3d_convert_hand_grenade(mesh, vertices, vertex_capacity,
                                        indices, index_capacity,
                                        GHG3D_PRESET_PINEAPPLE_CLASSIC);
    case 11:
        {
            unsigned short i;
            g3d_fx old_y;
            if (g3d_mesh_init(mesh, vertices, vertex_capacity,
                              indices, index_capacity) != G3D_OK) return 0;
            if (g3d_make_cylinder(mesh, g3d_fx_from_ratio(1L, 2L),
                                  g3d_fx_from_int(1L), 24U,
                                  g3d_color_rgba(120U, 225U, 255U, 150U))
                    != G3D_OK) return 0;
            g3d_mesh_gradient_y(mesh, -G3D_FX_HALF, G3D_FX_HALF,
                g3d_color_rgba(54U, 174U, 255U, 110U),
                g3d_color_rgba(255U, 255U, 255U, 210U));
            /* Shapes3D cylinders are local Y-up. Projectiles standardize
               their travel axis to local +Z, so rotate Y into Z without
               runtime trig or floating point. */
            for (i = 0U; i < mesh->vertex_count; ++i) {
                old_y = mesh->vertices[i].position.y;
                mesh->vertices[i].position.y = mesh->vertices[i].position.z;
                mesh->vertices[i].position.z = old_y;
                old_y = mesh->vertices[i].normal.y;
                mesh->vertices[i].normal.y = mesh->vertices[i].normal.z;
                mesh->vertices[i].normal.z = old_y;
            }
            return 1;
        }
    case 12:
        return b3d_convert_rocket(mesh, vertices, vertex_capacity,
                                  indices, index_capacity,
                                  RMESH_RPG7_PG7VR);
    case 13:
        if (g3d_mesh_init(mesh, vertices, vertex_capacity,
                          indices, index_capacity) != G3D_OK) return 0;
        return g3d_make_box(mesh,
                            G3D_FX_ONE, G3D_FX_ONE, G3D_FX_ONE,
                            g3d_color_rgba(255U, 146U, 42U, 178U))
               == G3D_OK;
    case 14:
        return b3d_make_energy_bolt(mesh, vertices, vertex_capacity, indices,
                                    index_capacity, g3d_fx_from_ratio(1L, 5L),
                                    g3d_fx_from_ratio(4L, 5L),
                                    g3d_color_rgba(120U, 220U, 255U, 255U));
    case 15:
        return b3d_make_energy_bolt(mesh, vertices, vertex_capacity, indices,
                                    index_capacity, g3d_fx_from_ratio(3L, 10L),
                                    G3D_FX_ONE,
                                    g3d_color_rgba(100U, 200U, 255U, 245U));
    case 16:
        return b3d_make_energy_bolt(mesh, vertices, vertex_capacity, indices,
                                    index_capacity, g3d_fx_from_ratio(2L, 5L),
                                    g3d_fx_from_ratio(6L, 5L),
                                    g3d_color_rgba(80U, 170U, 255U, 235U));
    case 17:
        return b3d_make_energy_bolt(mesh, vertices, vertex_capacity, indices,
                                    index_capacity, g3d_fx_from_ratio(1L, 2L),
                                    g3d_fx_from_ratio(3L, 2L),
                                    g3d_color_rgba(235U, 250U, 255U, 230U));
    default:
        break;
    }
    return 0;
}

const char *blank3d_projectile_mesh_name(int mesh_id)
{
    switch (mesh_id) {
    case 1: return "gbulletmesh89:pistol_projectile";
    case 2: return "gbulletmesh89:machinegun_projectile";
    case 3: return "gbulletmesh89:single_shotgun_pellet";
    case 4: return "gbulletmesh89:magnum_projectile";
    case 5: return "gbulletmesh89:sniper_projectile";
    case 6: return "rocketmeshes:grenade40_lv";
    case 7: return "rocketmeshes:survival_rpg7_cone";
    case 8: return "gbulletmesh89:machinegun_projectile_alias_for_gatling";
    case 9: return "giffany_shapes3d:slingshot_stone_primitive";
    case 10: return "ghandgrenade3d89:pineapple_classic_hand_grenade";
    case 11: return "giffany_shapes3d:shango_energy_cylinder";
    case 12: return "rocketmeshes:rpg7_pg7vr_homing";
    case 13: return "giffany_shapes3d:expandible_fire_cube";
    case 14: return "giffany_shapes3d:buster_lemon";
    case 15: return "giffany_shapes3d:buster_charge_1";
    case 16: return "giffany_shapes3d:buster_charge_2";
    case 17: return "giffany_shapes3d:buster_charge_3";
    default: return "projectile_mesh:invalid";
    }
}

int blank3d_casing_mesh_build(int mesh_id,
                              g3d_mesh *mesh,
                              g3d_vertex *vertices,
                              unsigned short vertex_capacity,
                              g3d_index *indices,
                              unsigned short index_capacity)
{
    int ammo_type;
    if (!mesh || !vertices || !indices) return 0;
    switch (mesh_id) {
    case 1: ammo_type = GBM_AMMO_PISTOL; break;
    case 2: ammo_type = GBM_AMMO_MACHINEGUN; break;
    case 3: ammo_type = GBM_AMMO_SHOTGUN; break;
    case 4: ammo_type = GBM_AMMO_MAGNUM; break;
    case 5: ammo_type = GBM_AMMO_SNIPER; break;
    default: return 0;
    }
    return b3d_convert_gbm_shell(mesh, vertices, vertex_capacity,
                                 indices, index_capacity, ammo_type);
}

const char *blank3d_casing_mesh_name(int mesh_id)
{
    switch (mesh_id) {
    case 1: return "gbulletmesh89:pistol_shell";
    case 2: return "gbulletmesh89:machinegun_shell";
    case 3: return "gbulletmesh89:shotgun_shell";
    case 4: return "gbulletmesh89:magnum_shell";
    case 5: return "gbulletmesh89:sniper_shell";
    default: return "casing_mesh:invalid";
    }
}
