#include "blank3d_objects.h"
#include <stdio.h>
#include <string.h>

static int logic_ticks;
static int draw_calls;
static int numbar_calls;

static void run_ddsl2(void *u, unsigned long id, void *native, const char *path)
{ (void)u; (void)id; (void)native; (void)path; ++logic_ticks; }
static void run_fpil(void *u, unsigned long id, void *native, const char *path)
{ (void)u; (void)id; (void)native; (void)path; ++logic_ticks; }
static void draw_mesh(void *u, unsigned long id, void *native,
                      const Blank3DObjectInit *init)
{ (void)u; (void)id; (void)native; if(init->mesh_kind != B3D_MESH_NONE) ++draw_calls; }
static void draw_script(void *u, unsigned long id, void *native,
                        const char *lang, const char *code, unsigned int n)
{ (void)u; (void)id; (void)native; (void)lang; (void)code; (void)n; }
static void handler(void *u, unsigned long id, void *native, const char *name)
{ (void)u; (void)id; (void)native; (void)name; }
static void invoke(void *u, unsigned long id, void *native, const char *name,
                   const gfo_value *argv, unsigned int argc)
{
    (void)u; (void)id; (void)native;
    if (name && strcmp(name, "draw_numbar") == 0) {
        if (argc != 1u || !argv) return;
        if ((argv[0].t == GFO_VAL_STR || argv[0].t == GFO_VAL_PATH) &&
            argv[0].v.s.len == strlen("config/hud/gameplay.ini") &&
            strncmp(argv[0].v.s.ptr, "config/hud/gameplay.ini",
                    argv[0].v.s.len) == 0)
            ++numbar_calls;
    }
}

int main(void)
{
    static Blank3DObjects objects;
    Blank3DObjectHost host;
    unsigned int slot;
    memset(&host, 0, sizeof(host));
    host.run_ddsl2 = run_ddsl2;
    host.run_fpil = run_fpil;
    host.draw_mesh = draw_mesh;
    host.draw_script = draw_script;
    host.handle = handler;
    host.invoke = invoke;
    blank3d_objects_init(&objects, &host);
    if (!blank3d_objects_spawn(&objects, "config/entities/player.ini", 1UL, 0, &slot)) {
        puts(blank3d_objects_status(&objects));
        return 1;
    }
    blank3d_objects_tick(&objects);
    blank3d_objects_render(&objects);
    if (logic_ticks != 1 || draw_calls != 1 || numbar_calls != 1) return 2;
    if (blank3d_objects_get(&objects, slot)->init.hp != 100) return 3;
    blank3d_objects_kill(&objects, slot);

    if (!blank3d_objects_spawn(&objects,
            "config/entities/armed_ally.ini", 1007UL, 0, &slot)) {
        puts(blank3d_objects_status(&objects));
        return 4;
    }
    blank3d_objects_tick(&objects);
    blank3d_objects_render(&objects);
    if (logic_ticks != 2 || draw_calls != 2 || numbar_calls != 1) return 5;
    if (blank3d_objects_get(&objects, slot)->init.hp != 60 ||
        blank3d_objects_get(&objects, slot)->init.mesh_kind !=
            B3D_MESH_COMPOSED ||
        blank3d_objects_get(&objects, slot)->init.logic_kind !=
            B3D_LOGIC_FPIL) return 6;
    blank3d_objects_kill(&objects, slot);
    puts("GFO player NumBar + armed ally FPIL/composed model test: OK");
    return 0;
}
