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
    const char *fire;
    const char *muzzle;
    const char *casing;
    const char *sky;
    const char *objects;
    const char *second_sky;

    if (!load_source("src/monika_blank3d.c")) return 1;

    fire = strstr(source_text, "event.type == GWP89_EVENT_FIRE_ACCEPTED");
    muzzle = strstr(source_text, "event.type == GWP89_EVENT_MUZZLE_REQUEST");
    casing = strstr(source_text, "event.type == GWP89_EVENT_CASING_REQUEST");
    if (!fire || !muzzle || !casing || !(fire < muzzle && muzzle < casing))
        return 2;

    if (!range_contains(fire, muzzle,
                        "event.actor_id == B3D_PLAYER_ACTOR_ID"))
        return 3;
    if (!range_contains(fire, muzzle, "g.muzzle_flash_ms ="))
        return 4;
    if (range_contains(muzzle, casing, "g.muzzle_flash_ms ="))
        return 5;

    sky = strstr(source_text, "blank3d_skybox89_gl_render(&g.skybox");
    objects = strstr(source_text, "blank3d_objects_render(&g.objects)");
    if (!sky || !objects || sky >= objects) return 6;
    second_sky = strstr(sky + 1, "blank3d_skybox89_gl_render(&g.skybox");
    if (second_sky) return 7;

    puts("PASS: local-player crosshair fire authority + sky-before-world order");
    return 0;
}
