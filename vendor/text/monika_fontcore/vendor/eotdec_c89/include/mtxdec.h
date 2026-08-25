#ifndef MTXDEC_H
#define MTXDEC_H

/*
   mtxdec - MicroType Express / MTX LZCOMP block unpacker
   C89, no malloc/free/realloc/heap, no float/double.

   Scope:
     - Parses the 10-byte MTX header embedded in EOT FontData.
     - Decompresses the three LZCOMP blocks into their CTF streams.
     - Does not yet rebuild final TTF tables from CTF; see docs/MTX_NOTES.md.
*/

#include <stdio.h>
#include "eotdec.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EOTDEC_MTX_MAX_COMPRESSED_BLOCK
#define EOTDEC_MTX_MAX_COMPRESSED_BLOCK (8UL * 1024UL * 1024UL)
#endif

#ifndef EOTDEC_MTX_MAX_COPY_DISTANCE
#define EOTDEC_MTX_MAX_COPY_DISTANCE (2UL * 1024UL * 1024UL)
#endif

#define EOTDEC_MTX_PRELOAD_SIZE (2UL * 32UL * 96UL + 4UL * 256UL)
#define EOTDEC_MTX_MAX_AHUFF_RANGE 512

#define EOTDEC_MTX_BLOCK_FONT_TABLES 1
#define EOTDEC_MTX_BLOCK_PUSH_DATA   2
#define EOTDEC_MTX_BLOCK_GLYPH_INSNS 3

struct eotdec_mtx_info {
    unsigned int version;
    unsigned long copy_limit;
    unsigned long offset_data2;
    unsigned long offset_data3;
    unsigned long mtx_size;
    unsigned long block_comp_size[3];
    unsigned long block_raw_size[3];
};

int eotdec_mtx_probe_file(FILE *fp, const struct eotdec_info *eot, struct eotdec_mtx_info *mtx);
int eotdec_mtx_unpack_blocks_file(FILE *fp, const struct eotdec_info *eot, const char *prefix, struct eotdec_mtx_info *mtx_out);
void eotdec_mtx_print_info(FILE *out, const struct eotdec_mtx_info *mtx);

#ifdef __cplusplus
}
#endif

#endif
