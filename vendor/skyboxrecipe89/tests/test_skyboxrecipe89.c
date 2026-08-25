#include "skyboxrecipe89.h"

#include <stdio.h>
#include <string.h>

typedef struct MemFileTag {
    const char *path;
    const char *text;
} MemFile;

typedef struct MemIOTag {
    const MemFile *files;
    int count;
} MemIO;

static int mem_load(void *user, const char *path, char *out,
                    unsigned int cap, unsigned int *out_size)
{
    MemIO *io;
    int i;
    unsigned int n;
    io = (MemIO *)user;
    if (!io || !path || !out || !out_size || cap == 0U) return 0;
    for (i = 0; i < io->count; ++i) {
        if (strcmp(io->files[i].path, path) != 0) continue;
        n = (unsigned int)strlen(io->files[i].text);
        if (n + 1U > cap) return 0;
        memcpy(out, io->files[i].text, n + 1U);
        *out_size = n;
        return 1;
    }
    return 0;
}

int main(void)
{
    static const MemFile files[] = {
        { "cfg/catalog.ini", "[recipes]\nnight=recipes/night.ini\n" },
        { "cfg/recipes/night.ini",
          "[recipe]\ninclude=../components/cube.ini\ninclude=../sources/night_atlas.ini\n"
          "[skybox]\nradius=96\n[dome]\ntop=10,20,30,80\n" },
        { "cfg/components/cube.ini",
          "[skybox]\nscreen=0\ncube=1\ndome=1\nradius=60\n"
          "[cube]\nflip_u_mask=3\n" },
        { "cfg/sources/night_atlas.ini",
          "[source]\ntype=atlas\nimage=night_cross\nlayout=cross4x3\nuv_inset_pixels=2\n" },
        { "cfg/family.ini",
          "[skybox]\nscreen=0\n[source]\ntype=family\nbase=Sky_Night01\nconvention=source\nextension=tga\n" },
        { "cfg/cycle_a.ini", "[recipe]\ninclude=cycle_b.ini\n" },
        { "cfg/cycle_b.ini", "[recipe]\ninclude=cycle_a.ini\n" }
    };
    MemIO mio;
    SBR89IOProvider provider;
    SBR89Workspace ws;
    SBR89Recipe recipe;
    mio.files = files;
    mio.count = (int)(sizeof(files) / sizeof(files[0]));
    provider.user = &mio;
    provider.load_text = mem_load;
    sbr89_workspace_init(&ws);
    if (!sbr89_load_named_recipe(&recipe, &ws, &provider,
                                 "cfg/catalog.ini", "night")) {
        printf("FAIL: %s line=%d\n", sbr89_last_error(&ws),
               sbr89_last_error_line(&ws));
        return 1;
    }
    if (strcmp(recipe.selected_name, "night") != 0) return 2;
    if (!recipe.cube_enabled || !recipe.dome_enabled || recipe.screen_enabled) return 3;
    if (recipe.radius_q16 != 96L * SBR89_FX_ONE) return 4;
    if (recipe.source_type != SBR89_SOURCE_ATLAS) return 5;
    if (recipe.layout != SBR89_LAYOUT_CROSS_4X3) return 6;
    if (strcmp(recipe.face_image[SBR89_FACE_PX], "night_cross") != 0) return 7;
    if (recipe.face_uv[SBR89_FACE_PX].u0_q16 != SBR89_FX_ONE / 2L) return 8;
    if (recipe.face_uv[SBR89_FACE_PX].v0_q16 != SBR89_FX_ONE / 3L) return 9;
    if (recipe.flip_u_mask != 3) return 10;
    if (recipe.uv_inset_pixels != 2) return 11;
    if (recipe.dome_top.r != 10U || recipe.dome_top.g != 20U ||
        recipe.dome_top.b != 30U || recipe.dome_top.a != 80U) return 12;
    if (!sbr89_load_recipe(&recipe, &ws, &provider, "cfg/family.ini")) return 13;
    if (strcmp(recipe.face_image[SBR89_FACE_PX], "Sky_Night01RT.tga") != 0) return 14;
    if (strcmp(recipe.face_image[SBR89_FACE_NZ], "Sky_Night01BK.tga") != 0) return 15;
    if (!recipe.cube_enabled) return 16;
    if (sbr89_load_recipe(&recipe, &ws, &provider, "cfg/cycle_a.ini")) return 17;
    if (!strstr(sbr89_last_error(&ws), "cycle")) return 18;
    puts("PASS: SkyboxRecipe89 catalog + matryoshka + atlas/family + cycle guard");
    return 0;
}
