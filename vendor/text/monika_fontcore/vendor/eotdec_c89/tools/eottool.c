#include "eotdec.h"
#include "mtxdec.h"
#include <stdio.h>
#include <string.h>

static void usage(void) {
    puts("eottool - C89 Embedded OpenType / MTX decoder/extractor");
    puts("usage:");
    puts("  eottool info <font.eot>");
    puts("  eottool extract <font.eot> <out.ttf|out.otf> [--loose]");
    puts("  eottool mtx-info <font.eot>");
    puts("  eottool mtx-unpack <font.eot> <out-prefix>");
    puts("");
    puts("Notes:");
    puts("  - no malloc/free/realloc/heap used");
    puts("  - raw EOT and XOR-protected EOT are extractable as SFNT");
    puts("  - MTX compressed EOT can be LZCOMP-unpacked into its 3 CTF blocks");
    puts("  - full CTF -> rebuilt TTF is documented as the next stage");
}

int main(int argc, char **argv) {
    FILE *in, *out;
    struct eotdec_info info;
    struct eotdec_sfnt_info sfnt;
    struct eotdec_mtx_info mtx;
    int r, strict;

    if (argc < 3) {
        usage();
        return 1;
    }

    in = fopen(argv[2], "rb");
    if (!in) {
        fprintf(stderr, "cannot open input: %s\n", argv[2]);
        return 2;
    }

    r = eotdec_probe_file(in, &info);
    if (r != EOTDEC_OK) {
        fprintf(stderr, "probe failed: %s\n", eotdec_errstr(r));
        fclose(in);
        return 3;
    }

    if (strcmp(argv[1], "info") == 0) {
        eotdec_print_info(stdout, &info);
        if ((info.flags & EOTDEC_FLAG_TTCOMPRESSED) != 0UL) {
            r = eotdec_mtx_probe_file(in, &info, &mtx);
            if (r == EOTDEC_OK) {
                puts("");
                eotdec_mtx_print_info(stdout, &mtx);
            } else {
                printf("\nMTX payload: %s\n", eotdec_errstr(r));
            }
        } else {
            r = eotdec_read_sfnt_info(in, &info, &sfnt);
            if (r == EOTDEC_OK) {
                puts("");
                eotdec_print_sfnt(stdout, &sfnt);
            } else {
                printf("\nSFNT payload: %s\n", eotdec_errstr(r));
            }
        }
        fclose(in);
        return 0;
    }

    if (strcmp(argv[1], "extract") == 0) {
        if (argc < 4) {
            usage();
            fclose(in);
            return 1;
        }
        strict = 1;
        if (argc >= 5 && strcmp(argv[4], "--loose") == 0) strict = 0;
        out = fopen(argv[3], "wb");
        if (!out) {
            fprintf(stderr, "cannot open output: %s\n", argv[3]);
            fclose(in);
            return 4;
        }
        r = eotdec_extract_file(in, out, &info, strict);
        fclose(out);
        fclose(in);
        if (r != EOTDEC_OK) {
            fprintf(stderr, "extract failed: %s\n", eotdec_errstr(r));
            return 5;
        }
        printf("extracted %lu bytes from offset %lu -> %s\n", info.font_data_size, info.font_offset, argv[3]);
        return 0;
    }

    if (strcmp(argv[1], "mtx-info") == 0) {
        r = eotdec_mtx_probe_file(in, &info, &mtx);
        fclose(in);
        if (r != EOTDEC_OK) {
            fprintf(stderr, "mtx-info failed: %s\n", eotdec_errstr(r));
            return 6;
        }
        eotdec_mtx_print_info(stdout, &mtx);
        return 0;
    }

    if (strcmp(argv[1], "mtx-unpack") == 0) {
        if (argc < 4) {
            usage();
            fclose(in);
            return 1;
        }
        r = eotdec_mtx_unpack_blocks_file(in, &info, argv[3], &mtx);
        fclose(in);
        if (r != EOTDEC_OK) {
            fprintf(stderr, "mtx-unpack failed: %s\n", eotdec_errstr(r));
            return 7;
        }
        eotdec_mtx_print_info(stdout, &mtx);
        printf("wrote: %s.block1_font_tables.ctf\n", argv[3]);
        printf("wrote: %s.block2_push_data.ctf\n", argv[3]);
        printf("wrote: %s.block3_glyph_insns.ctf\n", argv[3]);
        return 0;
    }

    usage();
    fclose(in);
    return 1;
}
