#ifndef EOTDEC_H
#define EOTDEC_H

/*
   eotdec_c89 - Embedded OpenType decoder / extractor
   Public domain / CC0-style. C89, no malloc/free/realloc/heap use.
*/

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EOTDEC_VERSION_10000 0x00010000UL
#define EOTDEC_VERSION_20001 0x00020001UL
#define EOTDEC_VERSION_20002 0x00020002UL

#define EOTDEC_MAGIC_PL      0x504CUL
#define EOTDEC_CS_XORKEY     0x50475342UL
#define EOTDEC_XORKEY        0x50U

#define EOTDEC_FLAG_SUBSET                   0x00000001UL
#define EOTDEC_FLAG_TTCOMPRESSED             0x00000004UL
#define EOTDEC_FLAG_FAILIFVARIATIONSIMULATED 0x00000010UL
#define EOTDEC_FLAG_EMBEDEUDC                0x00000020UL
#define EOTDEC_FLAG_VALIDATIONTESTS          0x00000040UL
#define EOTDEC_FLAG_WEBOBJECT                0x00000080UL
#define EOTDEC_FLAG_XORENCRYPTDATA           0x10000000UL

#define EOTDEC_MAX_TEXT_CHARS 255
#define EOTDEC_COPY_CHUNK     4096
#define EOTDEC_MAX_TABLES     256

#define EOTDEC_OK                 0
#define EOTDEC_ERR_IO            -1
#define EOTDEC_ERR_SHORT         -2
#define EOTDEC_ERR_BAD_MAGIC     -3
#define EOTDEC_ERR_BAD_VERSION   -4
#define EOTDEC_ERR_BAD_SIZE      -5
#define EOTDEC_ERR_ROOT_CHECKSUM -6
#define EOTDEC_ERR_COMPRESSED    -7
#define EOTDEC_ERR_UNSUPPORTED   -8
#define EOTDEC_ERR_SFNT          -9
#define EOTDEC_ERR_RANGE         -10
#define EOTDEC_ERR_MTX_CORRUPT   -11
#define EOTDEC_ERR_MTX_LIMIT     -12

struct eotdec_info {
    unsigned long eot_size;
    unsigned long file_size;
    unsigned long font_data_size;
    unsigned long version;
    unsigned long flags;
    unsigned long font_offset;
    unsigned long root_offset;
    unsigned long root_size;
    unsigned long root_checksum_stored;
    unsigned long root_checksum_calc;
    unsigned long eudc_offset;
    unsigned long eudc_size;
    unsigned long eudc_flags;
    unsigned long eudc_code_page;
    unsigned long check_sum_adjustment;
    unsigned long unicode_range[4];
    unsigned long codepage_range[2];
    unsigned long weight;
    unsigned int fs_type;
    unsigned int magic;
    unsigned int charset;
    unsigned int italic;
    char family[EOTDEC_MAX_TEXT_CHARS + 1];
    char style[EOTDEC_MAX_TEXT_CHARS + 1];
    char version_name[EOTDEC_MAX_TEXT_CHARS + 1];
    char full_name[EOTDEC_MAX_TEXT_CHARS + 1];
    char root_preview[EOTDEC_MAX_TEXT_CHARS + 1];
};

struct eotdec_sfnt_table {
    char tag[5];
    unsigned long checksum;
    unsigned long offset;
    unsigned long length;
};

struct eotdec_sfnt_info {
    unsigned long sfnt_version;
    unsigned int num_tables;
    unsigned int search_range;
    unsigned int entry_selector;
    unsigned int range_shift;
    struct eotdec_sfnt_table tables[EOTDEC_MAX_TABLES];
};

const char *eotdec_errstr(int code);
int eotdec_probe_file(FILE *fp, struct eotdec_info *out_info);
int eotdec_extract_file(FILE *in, FILE *out, const struct eotdec_info *info, int strict);
int eotdec_read_sfnt_info(FILE *fp, const struct eotdec_info *info, struct eotdec_sfnt_info *sfnt);
void eotdec_print_info(FILE *out, const struct eotdec_info *info);
void eotdec_print_sfnt(FILE *out, const struct eotdec_sfnt_info *sfnt);

#ifdef __cplusplus
}
#endif

#endif
