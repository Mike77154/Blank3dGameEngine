#include "blank3d_gproj_ammo_gbar.h"
#include "gproj2d89.h"

#define B3D_GP2D_PATH_CAP 64
#define B3D_GP2D_POINT_CAP 512
#define B3D_GP2D_VERTEX_CAP 128

static unsigned char b3d_rgba_r(unsigned long c) { return (unsigned char)((c >> 24) & 255UL); }
static unsigned char b3d_rgba_g(unsigned long c) { return (unsigned char)((c >> 16) & 255UL); }
static unsigned char b3d_rgba_b(unsigned long c) { return (unsigned char)((c >> 8) & 255UL); }
static unsigned char b3d_rgba_a(unsigned long c) { return (unsigned char)(c & 255UL); }

int blank3d_gproj_ammo_gbar_profile(int weapon_ammo_id)
{
    switch (weapon_ammo_id) {
    case 1: return GP2D_AMMO_PISTOL;
    case 2: return GP2D_AMMO_SHOTGUN_BUCKSHOT;
    case 3: return GP2D_AMMO_MAGNUM;
    case 4: return GP2D_AMMO_SNIPER;
    case 5: return GP2D_AMMO_GRENADE_LAUNCHER;
    case 6: return GP2D_AMMO_MISSILE;
    case 7: return GP2D_AMMO_MACHINE_GUN;
    case 9: return GP2D_AMMO_HAND_GRENADE;
    case 10: return GP2D_AMMO_MACHINE_GUN;
    case 12: return GP2D_AMMO_MISSILE;
    default: return GP2D_AMMO_PISTOL;
    }
}

void blank3d_gproj_ammo_gbar_init(Blank3DGProjAmmoGBar *bridge)
{
    if (!bridge) return;
    bridge->ammo_id = GP2D_AMMO_PISTOL;
    bridge->enabled = 1;
}

void blank3d_gproj_ammo_gbar_set_ammo(Blank3DGProjAmmoGBar *bridge,
                                      int weapon_ammo_id)
{
    if (!bridge) return;
    bridge->ammo_id = blank3d_gproj_ammo_gbar_profile(weapon_ammo_id);
}

void blank3d_gproj_ammo_gbar_attach(Blank3DGProjAmmoGBar *bridge,
                                    GBar89_Meter *meter)
{
    if (!meter) return;
    gbar89_set_unit_renderer(meter,
        bridge && bridge->enabled ? blank3d_gproj_ammo_gbar_render : 0,
        bridge);
}

static long b3d_gp2d_min(long a, long b) { return a < b ? a : b; }
static long b3d_gp2d_max(long a, long b) { return a > b ? a : b; }

void blank3d_gproj_ammo_gbar_render(void *user,
                                    const GBar89_RenderOps *ops,
                                    const GBar89_Rect *slot,
                                    unsigned long fill_rgba,
                                    unsigned long outline_rgba,
                                    int scale_percent)
{
    Blank3DGProjAmmoGBar *bridge;
    gp2d_path paths[B3D_GP2D_PATH_CAP];
    gp2d_vec2 points[B3D_GP2D_POINT_CAP];
    gp2d_scene scene;
    gp2d_transform transform;
    gp2d_draw_options options;
    GBar89_Vertex vertices[B3D_GP2D_VERTEX_CAP];
    int indices[(B3D_GP2D_VERTEX_CAP - 2) * 3];
    long min_x, min_y, max_x, max_y, span_x, span_y;
    long px, py;
    int draw_w, draw_h, draw_x, draw_y;
    int i, j, n, idx;
    const gp2d_path *path;
    unsigned long path_fill;
    gp2d_color gpfill;
    gp2d_color gpoutline;

    if (!user || !ops || !slot || slot->w <= 0 || slot->h <= 0) return;
    bridge = (Blank3DGProjAmmoGBar *)user;
    if (!bridge->enabled) return;

    gp2d_scene_init(&scene, paths, B3D_GP2D_PATH_CAP,
                    points, B3D_GP2D_POINT_CAP);
    transform = gp2d_transform_identity();
    transform.scale.x = gp2d_fx_from_int(64L);
    transform.scale.y = gp2d_fx_from_int(64L);
    options = gp2d_draw_options_default();
    gpfill = gp2d_color_rgba(b3d_rgba_r(fill_rgba), b3d_rgba_g(fill_rgba),
                            b3d_rgba_b(fill_rgba), b3d_rgba_a(fill_rgba));
    gpoutline = gp2d_color_rgba(b3d_rgba_r(outline_rgba), b3d_rgba_g(outline_rgba),
                               b3d_rgba_b(outline_rgba), b3d_rgba_a(outline_rgba));
    options.override_mask = GP2D_OVERRIDE_FILL | GP2D_OVERRIDE_OUTLINE;
    options.fill_override = gpfill;
    options.outline_override = gpoutline;
    options.outline_enabled = GP2D_TRUE;
    options.detail_enabled = GP2D_TRUE;
    options.outline_width = gp2d_fx_from_int(1L);
    if (gp2d_build_part(&scene, bridge->ammo_id, GP2D_PART_COMPLETE,
                        &transform, &options) != GP2D_OK ||
        scene.point_count == 0U) return;

    min_x = max_x = scene.points[0].x;
    min_y = max_y = scene.points[0].y;
    for (i = 1; i < (int)scene.point_count; ++i) {
        min_x = b3d_gp2d_min(min_x, scene.points[i].x);
        max_x = b3d_gp2d_max(max_x, scene.points[i].x);
        min_y = b3d_gp2d_min(min_y, scene.points[i].y);
        max_y = b3d_gp2d_max(max_y, scene.points[i].y);
    }
    span_x = max_x - min_x;
    span_y = max_y - min_y;
    if (span_x <= 0L || span_y <= 0L) return;
    if (scale_percent < 1) scale_percent = 1;
    if (scale_percent > 100) scale_percent = 100;
    draw_w = (slot->w * scale_percent) / 100;
    draw_h = (slot->h * scale_percent) / 100;
    if (draw_w < 1) draw_w = 1;
    if (draw_h < 1) draw_h = 1;

    /* Preserve the GProj silhouette aspect ratio.  Stretching X and Y
     * independently made tall cartridges look like fat GBar blocks and
     * visually hid the fact that the external vector provider was active. */
    if (span_x * (long)draw_h > span_y * (long)draw_w) {
        draw_h = (int)((span_y * (long)draw_w) / span_x);
        if (draw_h < 1) draw_h = 1;
    } else {
        draw_w = (int)((span_x * (long)draw_h) / span_y);
        if (draw_w < 1) draw_w = 1;
    }
    draw_x = slot->x + (slot->w - draw_w) / 2;
    draw_y = slot->y + (slot->h - draw_h) / 2;

    for (i = 0; i < (int)scene.path_count; ++i) {
        path = &scene.paths[i];
        n = (int)path->point_count;
        if (n < 2) continue;
        if (n > B3D_GP2D_VERTEX_CAP) n = B3D_GP2D_VERTEX_CAP;
        path_fill = fill_rgba;
        for (j = 0; j < n; ++j) {
            px = scene.points[path->first_point + (unsigned short)j].x - min_x;
            py = scene.points[path->first_point + (unsigned short)j].y - min_y;
            vertices[j].x = draw_x + (int)((px * draw_w) / span_x);
            vertices[j].y = draw_y + draw_h - (int)((py * draw_h) / span_y);
            vertices[j].u = 0;
            vertices[j].v = 0;
            vertices[j].color = path_fill;
        }
        if (path->style.fill_enabled && path->closed && n >= 3 &&
            ops->draw_triangles) {
            idx = 0;
            for (j = 1; j < n - 1; ++j) {
                indices[idx++] = 0;
                indices[idx++] = j;
                indices[idx++] = j + 1;
            }
            ops->draw_triangles(ops->user, vertices, n, indices, idx,
                                GBAR89_INVALID_SPRITE);
        }
        if (path->style.outline_enabled && ops->draw_line) {
            for (j = 0; j < n - 1; ++j)
                ops->draw_line(ops->user, vertices[j].x, vertices[j].y,
                               vertices[j + 1].x, vertices[j + 1].y,
                               outline_rgba);
            if (path->closed)
                ops->draw_line(ops->user, vertices[n - 1].x, vertices[n - 1].y,
                               vertices[0].x, vertices[0].y, outline_rgba);
        }
    }
}
