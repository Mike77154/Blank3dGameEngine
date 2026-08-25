#include <stdio.h>
#include "gcrosshair_recipe89.h"
#include "gcrosshair_base89.h"

static int open_count;
static int read_count;
static int close_count;

static void *io_open(void *user, const char *path)
{
    (void)user;
    open_count++;
    return (void *)fopen(path, "rb");
}

static int io_read_line(void *user, void *handle, char *buffer, int capacity)
{
    (void)user;
    read_count++;
    return fgets(buffer, capacity, (FILE *)handle) ? 1 : 0;
}

static void io_close(void *user, void *handle)
{
    (void)user;
    close_count++;
    fclose((FILE *)handle);
}

int main(void)
{
    GCB89_RecipeIoProvider io;
    io.open_read = io_open;
    io.read_line = io_read_line;
    io.close = io_close;
    open_count = 0;
    read_count = 0;
    close_count = 0;
    gcb89_recipe_set_io_provider(&io, 0);
    if (!gcb89_recipe_load_root("recipes/gcrosshair.ini")) {
        puts(gcb89_recipe_last_error());
        return 1;
    }
    if (gcb89_preset_count() != 192) return 1;
    if (open_count <= 0 || read_count <= 0 || close_count != open_count)
        return 1;
    gcb89_recipe_clear_io_provider();
    puts("gcrosshair_recipe89 IO provider: OK");
    return 0;
}
