#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../pcx.h"

static void usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s [--json|--text] [--strict] <file.pcx>\n",
            argv0 ? argv0 : "pcx_diag_cli");
}

int main(int argc, char **argv)
{
    const char *path = NULL;
    int json = 0;
    int strict = 0;
    int i;
    int rc;
    PCXFileInfo info;

    for (i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--json") == 0)
        {
            json = 1;
        }
        else if (strcmp(argv[i], "--text") == 0)
        {
            json = 0;
        }
        else if (strcmp(argv[i], "--strict") == 0)
        {
            strict = 1;
        }
        else if (argv[i][0] == '-')
        {
            usage(argv[0]);
            return 2;
        }
        else
        {
            path = argv[i];
        }
    }

    if (path == NULL)
    {
        usage(argv[0]);
        return 2;
    }

    rc = pcx_inspect_file_ex(path, &info);
    if (rc != PCX_OK)
    {
        fprintf(stderr, "pcx_inspect_file_ex failed: %s\n", pcx_result_to_string((PCXResult)rc));
        return 1;
    }

    {
        PCXImage img;
        PCXFileInfo decodedInfo;
        pcx_image_init(&img);
        rc = pcx_load_with_info(path, &img, &decodedInfo);
        if (rc == PCX_OK)
        {
            info = decodedInfo;
            pcx_image_release(&img);
        }
        else
        {
            pcx_image_release(&img);
        }
    }

    rc = json ? pcx_write_diagnostics_json(stdout, &info)
              : pcx_write_diagnostics_text(stdout, &info);
    if (rc != PCX_OK)
    {
        fprintf(stderr, "report output failed: %s\n", pcx_result_to_string((PCXResult)rc));
        return 1;
    }

    if (strict && !info.strictHeaderPasses)
    {
        return 3;
    }
    return 0;
}
