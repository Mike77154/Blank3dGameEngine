#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../pcx.h"

static void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [--json] <file.pcx>\n", argv0 ? argv0 : "pcx_compat_cli");
}

int main(int argc, char **argv)
{
    const char *path = NULL;
    int json = 0;
    int i;
    int inspect_rc;
    int rgb_rc = PCX_ERR_NULL_POINTER;
    int indexed_rc = PCX_ERR_NULL_POINTER;
    PCXFileInfo info;
    PCXImage rgb;
    PCXIndexedImage indexed;

    for (i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--json") == 0) json = 1;
        else if (argv[i][0] == '-') { usage(argv[0]); return 2; }
        else path = argv[i];
    }
    if (path == NULL) { usage(argv[0]); return 2; }

    inspect_rc = pcx_inspect_file_ex(path, &info);
    pcx_image_init(&rgb);
    pcx_indexed_image_init(&indexed);
    if (inspect_rc == PCX_OK)
    {
        rgb_rc = pcx_load_with_info(path, &rgb, &info);
        indexed_rc = pcx_load_indexed(path, &indexed);
    }

    if (json)
    {
        printf("{\n");
        printf("  \"path\": \"%s\",\n", path);
        printf("  \"inspect_ok\": %s,\n", (inspect_rc == PCX_OK) ? "true" : "false");
        printf("  \"inspect_status\": \"%s\",\n", pcx_result_to_string((PCXResult)inspect_rc));
        printf("  \"rgb_ok\": %s,\n", (rgb_rc == PCX_OK) ? "true" : "false");
        printf("  \"rgb_status\": \"%s\",\n", pcx_result_to_string((PCXResult)rgb_rc));
        printf("  \"indexed_ok\": %s,\n", (indexed_rc == PCX_OK) ? "true" : "false");
        printf("  \"indexed_status\": \"%s\",\n", pcx_result_to_string((PCXResult)indexed_rc));
        if (inspect_rc == PCX_OK)
        {
            printf("  \"strict_ok\": %s,\n", info.strictHeaderPasses ? "true" : "false");
            printf("  \"width\": %d,\n", info.width);
            printf("  \"height\": %d,\n", info.height);
            printf("  \"total_bits_per_pixel\": %d,\n", info.totalBitsPerPixel);
            printf("  \"warning_mask\": %u\n", (unsigned int)info.diagnostics.warningMask);
        }
        else
        {
            printf("  \"strict_ok\": false,\n");
            printf("  \"width\": null,\n");
            printf("  \"height\": null,\n");
            printf("  \"total_bits_per_pixel\": null,\n");
            printf("  \"warning_mask\": 0\n");
        }
        printf("}\n");
    }
    else
    {
        printf("inspect=%s rgb=%s indexed=%s\n",
               pcx_result_to_string((PCXResult)inspect_rc),
               pcx_result_to_string((PCXResult)rgb_rc),
               pcx_result_to_string((PCXResult)indexed_rc));
    }

    pcx_image_release(&rgb);
    pcx_indexed_image_release(&indexed);
    return 0;
}
