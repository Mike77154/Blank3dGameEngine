/* Generate one PPM per preset using gscopepresets89 itself. */
#include <stdio.h>
#include "../include/gscopepresets89.h"

int gpr89_render_preset(const gsvp89_preset *preset, const char *filename);

int main(void)
{
    short i;
    char path[256];
    const gsvp89_preset *preset;
    for (i = 0; i < gsvp89_count(); ++i) {
        preset = gsvp89_get(i);
        if (!preset) return 2;
        sprintf(path, "generated_preview/%02d_%s.ppm", (int)i, preset->name);
        if (!gpr89_render_preset(preset, path)) return 3;
        printf("%02d %s\n", (int)i, preset->name);
    }
    return 0;
}
