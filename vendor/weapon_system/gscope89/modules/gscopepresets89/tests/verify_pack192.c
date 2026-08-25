#include <stdio.h>
#include <string.h>
#include "gscopepresets89.h"

int main(void)
{
    short i;
    short count;
    const gsvp89_preset *preset;
    const gsvp89_preset *found;

    gsvp89_set_catalog_path("config/reticles/catalog.ini");
    count = gsvp89_count();
    if (count != (short)192) {
        fprintf(stderr, "expected 192 presets, got %d\n", (int)count);
        return 1;
    }

    for (i = 0; i < count; ++i) {
        preset = gsvp89_get(i);
        if (!preset) {
            fprintf(stderr, "null preset at id %d\n", (int)i);
            return 2;
        }
        if (preset->id != i) {
            fprintf(stderr, "id mismatch at %d\n", (int)i);
            return 3;
        }
        if (!preset->name || !preset->name[0]) {
            fprintf(stderr, "missing name at id %d\n", (int)i);
            return 4;
        }
        if (!preset->shapes || preset->shape_count <= 0) {
            fprintf(stderr, "empty geometry at id %d (%s)\n",
                    (int)i, preset->name);
            return 5;
        }
        found = gsvp89_find(preset->name);
        if (found != preset) {
            fprintf(stderr, "find mismatch at id %d (%s)\n",
                    (int)i, preset->name);
            return 6;
        }
    }

    {
        const gsvp89_preset *selected;
        selected = gsvp89_select_from_ini("config/reticles/active.ini", "reticle");
        if (!selected || strcmp(selected->name, "svd_pso1_dragunov") != 0) {
            fprintf(stderr, "selector did not resolve svd_pso1_dragunov\n");
            return 7;
        }
    }

    printf("verified %d INI presets; IDs, names, vector geometry and selector are valid\n",
           (int)count);
    return 0;
}
