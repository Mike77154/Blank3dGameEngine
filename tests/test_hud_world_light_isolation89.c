#include <stdio.h>
#include <string.h>

#define B3D_SOURCE_CAP 1048576U

static char source_text[B3D_SOURCE_CAP];

static int load_source(const char *path)
{
    FILE *fp;
    size_t n;
    fp = fopen(path, "rb");
    if (!fp) return 0;
    n = fread(source_text, 1U, B3D_SOURCE_CAP - 1U, fp);
    fclose(fp);
    if (n == 0U || n >= B3D_SOURCE_CAP - 1U) return 0;
    source_text[n] = '\0';
    return 1;
}

static int range_contains(const char *begin, const char *end,
                          const char *needle)
{
    const char *p;
    if (!begin || !end || !needle || begin >= end) return 0;
    p = strstr(begin, needle);
    return p != 0 && p < end;
}

int main(void)
{
    const char *begin;
    const char *end;
    const char *hud_call;

    if (!load_source("src/blank3d_hud.c")) return 1;
    begin = strstr(source_text, "static void b3d_begin_2d");
    end = strstr(source_text, "static void b3d_end_2d");
    if (!begin || !end || begin >= end) return 2;
    if (!range_contains(begin, end, "glDisable(GL_LIGHTING)")) return 3;
    if (!range_contains(begin, end, "glDisable(GL_TEXTURE_2D)")) return 4;
    if (!range_contains(begin, end, "glDisable(GL_DEPTH_TEST)")) return 5;
    if (!range_contains(begin, end,
                        "glColor4ub(255U, 255U, 255U, 255U)")) return 6;

    begin = end;
    end = strstr(begin + 1, "static int b3d_ecg_color_equal");
    if (!end) return 7;
    if (!range_contains(begin, end, "glDisable(GL_TEXTURE_2D)")) return 8;
    if (!range_contains(begin, end, "glEnable(GL_LIGHTING)")) return 9;
    if (!range_contains(begin, end,
                        "glColor4ub(255U, 255U, 255U, 255U)")) return 10;

    if (!load_source("src/blank3d_image_gl.c")) return 11;
    begin = strstr(source_text, "void blank3d_image_gl_begin_overlay");
    end = strstr(source_text, "void blank3d_image_gl_end_overlay");
    if (!begin || !end || begin >= end) return 12;
    if (!range_contains(begin, end, "glDisable(GL_LIGHTING)")) return 13;
    if (!range_contains(begin, end, "glDisable(GL_TEXTURE_2D)")) return 14;
    if (!range_contains(begin, end,
                        "glColor4ub(255U, 255U, 255U, 255U)")) return 15;

    begin = end;
    end = strstr(begin + 1, "int blank3d_image_gl_draw_subrect");
    if (!end) return 16;
    if (!range_contains(begin, end, "glEnable(GL_LIGHTING)")) return 17;

    if (!load_source("src/engine_bridge.c")) return 18;
    begin = strstr(source_text, "bridge_gl_set_muzzle_light");
    if (!begin || !strstr(begin, "glEnable(GL_LIGHT1)")) return 19;

    if (!load_source("src/monika_blank3d.c")) return 20;
    hud_call = strstr(source_text, "blank3d_hud_draw(&g.hud");
    if (!hud_call) return 21;
    if (!strstr(hud_call, "g.player_hp")) return 22;
    if (!strstr(hud_call, "blank3d_systems_clip(&g.systems)")) return 23;
    if (!strstr(hud_call, "blank3d_systems_reserve(&g.systems)")) return 24;

    if (!load_source("config/hud/gameplay.ini")) return 25;
    begin = strstr(source_text, "[numbar player_health]");
    end = strstr(source_text, "[numbar player_health_vertical]");
    if (!begin || !end || begin >= end) return 26;
    if (!range_contains(begin, end, "bind.value=player.health")) return 27;
    if (range_contains(begin, end, "gameplay.threat")) return 28;

    puts("PASS: HUD/overlay ignore world muzzle lighting; player HUD data stays local");
    return 0;
}
