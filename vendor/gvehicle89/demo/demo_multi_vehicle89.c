#include <stdio.h>
#include "gveh_engine_adapter.h"
#include "gveh_profile_bank.h"

static void plot_point(unsigned char img[128][128], gveh_fx x, gveh_fx z, unsigned char v)
{
    int px;
    int pz;
    px = 64 + gveh_fx_to_int(x);
    pz = 110 - gveh_fx_to_int(z);
    if (px < 0 || px >= 128 || pz < 0 || pz >= 128) return;
    img[pz][px] = v;
}

static void write_ppm(const char *path, unsigned char img[128][128])
{
    FILE *f;
    int y;
    int x;
    unsigned char p;
    f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n128 128\n255\n");
    y = 0;
    while (y < 128) {
        x = 0;
        while (x < 128) {
            p = img[y][x];
            if (p == 0) { fputc(18, f); fputc(18, f); fputc(22, f); }
            else if (p == 1) { fputc(240, f); fputc(230, f); fputc(110, f); }
            else if (p == 2) { fputc(110, f); fputc(220, f); fputc(255, f); }
            else { fputc(255, f); fputc(130, f); fputc(90, f); }
            x++;
        }
        y++;
    }
    fclose(f);
}

static void drive_input(gveh_input *in, gveh_i32 tick, gveh_i16 lane)
{
    gveh_input_clear(in);
    in->throttle = GVEH_FX_ONE;
    if (lane == 0) {
        if (tick > 80 && tick < 260) in->steer = GVEH_FX_ONE / 3;
        if (tick >= 260 && tick < 430) in->steer = -GVEH_FX_ONE / 4;
        if (tick > 120 && tick < 180) in->nitro = GVEH_FX_ONE;
    } else if (lane == 1) {
        if (tick > 60 && tick < 220) in->steer = -GVEH_FX_ONE / 4;
        if (tick >= 220 && tick < 440) in->steer = GVEH_FX_ONE / 3;
        if (tick > 250 && tick < 330) in->handbrake = GVEH_FX_ONE / 2;
    } else {
        in->throttle = GVEH_FX_ONE / 2;
        if (tick > 100 && tick < 520) in->steer = GVEH_FX_ONE / 6;
        if (tick > 500) in->brake = GVEH_FX_ONE / 2;
    }
}

int main(void)
{
    gveh_vehicle_world world;
    gveh_profile p0;
    gveh_profile p1;
    gveh_profile p2;
    gveh_input *in0;
    gveh_input *in1;
    gveh_input *in2;
    gveh_vehicle *v0;
    gveh_vehicle *v1;
    gveh_vehicle *v2;
    gveh_i16 id0;
    gveh_i16 id1;
    gveh_i16 id2;
    gveh_i32 i;
    gveh_fx dt;
    FILE *csv;
    unsigned char img[128][128];
    int x;
    int y;

    gveh_vehicle_world_init(&world);
    gveh_profile_bank_make_by_name("warthog89", &p0);
    gveh_profile_bank_make_by_name("rally89", &p1);
    gveh_profile_bank_make_by_name("tank_lite89", &p2);
    p0.collision_radius = gveh_fx_from_int(2);
    p1.collision_radius = gveh_fx_from_int(2);
    p2.collision_radius = gveh_fx_from_int(3);

    id0 = gveh_vehicle_world_spawn(&world, &p0, gveh_v3(-gveh_fx_from_int(6), gveh_fx_from_int(2), 0), 100);
    id1 = gveh_vehicle_world_spawn(&world, &p1, gveh_v3(gveh_fx_from_int(6), gveh_fx_from_int(2), 0), 101);
    id2 = gveh_vehicle_world_spawn(&world, &p2, gveh_v3(0, gveh_fx_from_int(2), -gveh_fx_from_int(12)), 102);

    in0 = gveh_vehicle_world_input(&world, id0);
    in1 = gveh_vehicle_world_input(&world, id1);
    in2 = gveh_vehicle_world_input(&world, id2);
    dt = GVEH_FX_ONE / 30;

    csv = fopen("demo_multi_vehicle89.csv", "w");
    if (!csv) return 2;
    fprintf(csv, "tick,id,name,pos_x,pos_y,pos_z,vel_x,vel_y,vel_z,yaw,collisions,fx_count\n");
    y = 0;
    while (y < 128) { x = 0; while (x < 128) { img[y][x] = 0; x++; } y++; }

    i = 0;
    while (i < 720) {
        drive_input(in0, i, 0);
        drive_input(in1, i, 1);
        drive_input(in2, i, 2);
        gveh_vehicle_world_step(&world, dt);
        v0 = gveh_vehicle_world_get(&world, id0);
        v1 = gveh_vehicle_world_get(&world, id1);
        v2 = gveh_vehicle_world_get(&world, id2);
        if (v0 != 0) {
            fprintf(csv, "%ld,%d,%s,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d,%d\n", (long)i, (int)id0, v0->profile.name, (long)gveh_fx_to_int(v0->body.pos.x), (long)gveh_fx_to_int(v0->body.pos.y), (long)gveh_fx_to_int(v0->body.pos.z), (long)gveh_fx_to_int(v0->body.vel.x), (long)gveh_fx_to_int(v0->body.vel.y), (long)gveh_fx_to_int(v0->body.vel.z), (long)v0->body.yaw, (int)world.collision_hits, (int)v0->fxq.count);
            if ((i % 3) == 0) plot_point(img, v0->body.pos.x, v0->body.pos.z, 1);
        }
        if (v1 != 0) {
            fprintf(csv, "%ld,%d,%s,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d,%d\n", (long)i, (int)id1, v1->profile.name, (long)gveh_fx_to_int(v1->body.pos.x), (long)gveh_fx_to_int(v1->body.pos.y), (long)gveh_fx_to_int(v1->body.pos.z), (long)gveh_fx_to_int(v1->body.vel.x), (long)gveh_fx_to_int(v1->body.vel.y), (long)gveh_fx_to_int(v1->body.vel.z), (long)v1->body.yaw, (int)world.collision_hits, (int)v1->fxq.count);
            if ((i % 3) == 0) plot_point(img, v1->body.pos.x, v1->body.pos.z, 2);
        }
        if (v2 != 0) {
            fprintf(csv, "%ld,%d,%s,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d,%d\n", (long)i, (int)id2, v2->profile.name, (long)gveh_fx_to_int(v2->body.pos.x), (long)gveh_fx_to_int(v2->body.pos.y), (long)gveh_fx_to_int(v2->body.pos.z), (long)gveh_fx_to_int(v2->body.vel.x), (long)gveh_fx_to_int(v2->body.vel.y), (long)gveh_fx_to_int(v2->body.vel.z), (long)v2->body.yaw, (int)world.collision_hits, (int)v2->fxq.count);
            if ((i % 3) == 0) plot_point(img, v2->body.pos.x, v2->body.pos.z, 3);
        }
        i++;
    }
    fclose(csv);
    write_ppm("demo_multi_vehicle89.ppm", img);
    printf("gvehicle89 multi demo ok: active=%d profiles=%d\n", (int)world.active_count, (int)gveh_profile_bank_count());
    return 0;
}
