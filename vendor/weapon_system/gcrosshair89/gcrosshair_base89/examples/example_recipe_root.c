#include <stdio.h>
#include "gcrosshair_base89.h"

int main(int argc, char **argv)
{
    const char *root;
    root = argc > 1 ? argv[1] : "recipes/gcrosshair.ini";
    if (!gcb89_recipe_load_root(root)) {
        fprintf(stderr, "load failed: %s\n", gcb89_recipe_last_error());
        return 1;
    }
    printf("loaded %d presets from %s\n",
           gcb89_preset_count(), gcb89_recipe_root_path());
    return 0;
}
