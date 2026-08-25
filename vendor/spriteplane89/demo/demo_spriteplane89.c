/* demo_spriteplane89.c - compile/run smoke test. Exports OBJ mesh.
   Build: gcc -std=gnu89 -Wall -Wextra -I../include ../src/spriteplane89.c demo_spriteplane89.c -o demo_spriteplane89
*/
#include <stdio.h>
#include "spriteplane89.h"

static int demo_cull_far(sprpl89_ctx *ctx, int sprite_index, const sprpl89_sprite *sprite, void *user)
{
    (void)ctx; (void)sprite_index; (void)sprite; (void)user;
    return 0;
}

static void demo_mutate(sprpl89_ctx *ctx, int sprite_index, sprpl89_sprite *sprite_copy, void *user)
{
    (void)ctx; (void)sprite_index; (void)user;
    if (sprite_copy->user0 == 777u) sprite_copy->color = sprpl89_rgba(255, 230, 170, 230);
}

static void write_obj(const char *path, const sprpl89_emit *emit)
{
    FILE *f;
    int i;
    f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "# spriteplane89 v2 demo OBJ - positions and UVs\n");
    for (i = 0; i < emit->vert_count; ++i) {
        fprintf(f, "v %d %d %d\n",
            sprpl89_fx_to_int(emit->verts[i].pos.x),
            sprpl89_fx_to_int(emit->verts[i].pos.y),
            sprpl89_fx_to_int(emit->verts[i].pos.z));
    }
    for (i = 0; i < emit->vert_count; ++i) {
        fprintf(f, "vt %d %d\n",
            sprpl89_fx_to_int(emit->verts[i].u * 100),
            sprpl89_fx_to_int(emit->verts[i].v * 100));
    }
    for (i = 0; i < emit->tri_count; ++i) {
        fprintf(f, "f %u/%u %u/%u %u/%u\n",
            (unsigned)(emit->tris[i].a + 1), (unsigned)(emit->tris[i].a + 1),
            (unsigned)(emit->tris[i].b + 1), (unsigned)(emit->tris[i].b + 1),
            (unsigned)(emit->tris[i].c + 1), (unsigned)(emit->tris[i].c + 1));
    }
    fclose(f);
}

static sprpl89_sprite make_base_sprite(int frame0)
{
    sprpl89_sprite spr;
    spr.active = 0;
    spr.flags = SP89_FLAG_ALPHA_BLEND | SP89_FLAG_DEPTH_TEST;
    spr.xflags = 0;
    spr.billboard_mode = SP89_BILLBOARD_Y_AXIS;
    spr.frame_mode = SP89_FRAME_STATIC;
    spr.anchor_mode = SP89_ANCHOR_BOTTOM_CENTER;
    spr.render_queue = SP89_QUEUE_TRANSPARENT;
    spr.blend_mode = SP89_BLEND_ALPHA;
    spr.sort_layer = 0;
    spr.sort_order = 0;
    spr.pos = sprpl89_v3(0, 0, sprpl89_fx_from_int(3));
    spr.axis_right = sprpl89_v3(SP89_FX_ONE, 0, 0);
    spr.axis_up = sprpl89_v3(0, SP89_FX_ONE, 0);
    spr.axis_lock = sprpl89_v3(0, SP89_FX_ONE, 0);
    spr.velocity = sprpl89_v3(0, 0, 0);
    spr.surface_right = sprpl89_v3(SP89_FX_ONE, 0, 0);
    spr.surface_up = sprpl89_v3(0, SP89_FX_ONE, 0);
    spr.width = sprpl89_fx_from_int(2);
    spr.height = sprpl89_fx_from_int(3);
    spr.scale_x = SP89_FX_ONE;
    spr.scale_y = SP89_FX_ONE;
    spr.offset_x = 0; spr.offset_y = 0; spr.offset_z = 0; spr.depth_bias = 0;
    spr.cull_distance = 0;
    spr.fade_near_distance = 0;
    spr.fade_far_distance = 0;
    spr.yaw_u16 = 0;
    spr.base_frame = (sprpl89_u16)frame0;
    spr.frame_count = 1;
    spr.view_count = 1;
    spr.anim_fps_fx8 = 0;
    spr.anim_phase_fx8 = 0;
    spr.texture_page = 0;
    spr.material_id = 1;
    spr.color = sprpl89_rgba(255, 255, 255, 220);
    spr.custom_anchor_x = SP89_FX_HALF;
    spr.custom_anchor_y = 0;
    spr.user0 = 0;
    spr.user1 = 0;
    return spr;
}

int main(void)
{
    sprpl89_ctx ctx;
    sprpl89_camera cam;
    sprpl89_sprite spr;
    sprpl89_emit emit;
    sprpl89_hooks hooks;
    sprpl89_pick_hit hit;
    int frame0, frame_panel, frame_shadow;
    int sprite_id;
    int font_id;

    sprpl89_init(&ctx);

    cam.pos = sprpl89_v3(sprpl89_fx_from_int(0), sprpl89_fx_from_int(2), sprpl89_fx_from_int(-8));
    cam.right = sprpl89_v3(SP89_FX_ONE, 0, 0);
    cam.up = sprpl89_v3(0, SP89_FX_ONE, 0);
    cam.forward = sprpl89_v3(0, 0, SP89_FX_ONE);
    cam.world_up = sprpl89_v3(0, SP89_FX_ONE, 0);
    sprpl89_set_camera(&ctx, &cam);
    sprpl89_set_tick_ms(&ctx, 250);

    hooks.user = 0;
    hooks.cull_sprite = demo_cull_far;
    hooks.resolve_frame = 0;
    hooks.mutate_sprite = demo_mutate;
    hooks.post_emit = 0;
    hooks.depth_fade = 0;
    sprpl89_set_hooks(&ctx, &hooks);

    frame0 = sprpl89_add_frame_px(&ctx, 256, 256, 0, 0, 64, 64, 0, 100);
    frame_panel = sprpl89_add_frame_px(&ctx, 256, 256, 64, 0, 64, 64, 0, 101);
    frame_shadow = sprpl89_add_frame_px(&ctx, 256, 256, 128, 0, 64, 64, 0, 102);

    font_id = sprpl89_add_font(&ctx, 8, 4, 2);
    sprpl89_font_set_ascii_grid(&ctx, font_id, 256, 256, 0, 32, 16, 6, 8, 8, 8);

    spr = make_base_sprite(frame0);
    spr.user0 = 777u;
    sprite_id = sprpl89_spawn_sprite(&ctx, &spr);
    if (sprite_id < 0) return 2;

    sprpl89_add_panel_9slice(&ctx, sprite_id, frame_panel,
        -sprpl89_fx_from_int(1), sprpl89_fx_from_int(1),
         sprpl89_fx_from_int(1), sprpl89_fx_from_int(2),
         sprpl89_fx_from_int(1) / 8,
         sprpl89_rgba(50, 80, 180, 180));

    sprpl89_add_vector_rect(&ctx, sprite_id,
        -SP89_FX_HALF, sprpl89_fx_from_int(1),
         SP89_FX_HALF, sprpl89_fx_from_int(2),
        sprpl89_rgba(255, 80, 20, 180));
    sprpl89_add_vector_line(&ctx, sprite_id,
        -SP89_FX_HALF, sprpl89_fx_from_int(2),
         SP89_FX_HALF, sprpl89_fx_from_int(1),
        sprpl89_fx_from_int(1) / 16,
        sprpl89_rgba(255, 255, 255, 255));
    sprpl89_add_vector_circle(&ctx, sprite_id, 0, sprpl89_fx_from_int(2), sprpl89_fx_from_int(1) / 4, sprpl89_rgba(255, 255, 0, 160));
    sprpl89_add_text(&ctx, sprite_id, font_id, "SPR89", -SP89_FX_HALF, sprpl89_fx_from_int(2), SP89_FX_ONE / 16, SP89_FX_ONE / 16, sprpl89_rgba(255,255,255,255), SP89_TEXT_LEFT);
    sprpl89_add_shadow_blob(&ctx, sprite_id, frame_shadow, sprpl89_fx_from_int(1), sprpl89_fx_from_int(1) / 2, -sprpl89_fx_from_int(1) / 64, sprpl89_rgba(0,0,0,96));

    spr = make_base_sprite(frame0);
    spr.pos = sprpl89_v3(sprpl89_fx_from_int(3), sprpl89_fx_from_int(1), sprpl89_fx_from_int(5));
    spr.width = sprpl89_fx_from_int(1);
    spr.height = sprpl89_fx_from_int(1);
    spr.xflags = SP89_XFLAG_POINT_COMMAND;
    sprpl89_spawn_sprite(&ctx, &spr);

    sprpl89_emit_begin(&emit);
    sprpl89_emit_all_sorted(&ctx, &emit);

    sprpl89_pick_all_ray(&ctx, cam.pos, cam.forward, &hit);

    printf("spriteplane89 v2 emitted verts=%d tris=%d packets=%d points=%d culled=%d hit=%d\n",
        emit.vert_count, emit.tri_count, emit.packet_count, emit.point_count, emit.culled_sprites, hit.hit);
    printf("dropped sprites=%d vectors=%d text=%d panels=%d shadows=%d points=%d\n",
        emit.dropped_sprites, emit.dropped_vector_cmds, emit.dropped_text_cmds, emit.dropped_panel_cmds, emit.dropped_shadow_cmds, emit.dropped_point_cmds);
    write_obj("out/spriteplane89_v2_demo.obj", &emit);
    return 0;
}
