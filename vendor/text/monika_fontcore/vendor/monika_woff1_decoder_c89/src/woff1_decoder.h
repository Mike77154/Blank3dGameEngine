#ifndef WOFF1_DECODER_H
#define WOFF1_DECODER_H

#include "woff1_types.h"
#include "woff1_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WOFF1_TAG(a,b,c,d) ((((woff1_u32)(a)) << 24) | (((woff1_u32)(b)) << 16) | (((woff1_u32)(c)) << 8) | ((woff1_u32)(d)))
#define WOFF1_SIGNATURE WOFF1_TAG('w','O','F','F')

typedef enum woff1_result_e {
    WOFF1_OK = 0,
    WOFF1_ERR_NULL = -100,
    WOFF1_ERR_TOO_SMALL = -101,
    WOFF1_ERR_BAD_SIGNATURE = -102,
    WOFF1_ERR_BAD_HEADER = -103,
    WOFF1_ERR_TOO_MANY_TABLES = -104,
    WOFF1_ERR_RANGE = -105,
    WOFF1_ERR_OVERLAP = -106,
    WOFF1_ERR_UNSORTED_TAGS = -107,
    WOFF1_ERR_BAD_COMPRESSION_LENGTH = -108,
    WOFF1_ERR_OUTPUT_TOO_SMALL = -109,
    WOFF1_ERR_TOTAL_SFNT_SIZE = -110,
    WOFF1_ERR_INFLATE = -111,
    WOFF1_ERR_CHECKSUM = -112,
    WOFF1_ERR_NO_HEAD = -113,
    WOFF1_ERR_PRIVATE_RANGE = -114,
    WOFF1_ERR_METADATA_RANGE = -115
} woff1_result;

typedef struct woff1_report_s {
    woff1_u32 flavor;
    woff1_u16 num_tables;
    woff1_u16 major_version;
    woff1_u16 minor_version;
    woff1_u32 woff_length;
    woff1_u32 total_sfnt_size;
    woff1_u32 metadata_offset;
    woff1_u32 metadata_length;
    woff1_u32 metadata_orig_length;
    woff1_u32 private_offset;
    woff1_u32 private_length;
    woff1_u32 repaired_checksum_adjustment;
    int last_inflate_error;
} woff1_report;

void woff1_report_clear(woff1_report *report);
int woff1_decode_to_sfnt(const woff1_u8 *woff, woff1_u32 woff_size,
                         woff1_u8 *sfnt, woff1_u32 sfnt_cap,
                         woff1_u32 *sfnt_size,
                         woff1_report *report);
int woff1_decode_metadata_xml(const woff1_u8 *woff, woff1_u32 woff_size,
                              woff1_u8 *out, woff1_u32 out_cap,
                              woff1_u32 *out_len,
                              woff1_report *report);
int woff1_copy_private_data(const woff1_u8 *woff, woff1_u32 woff_size,
                            woff1_u8 *out, woff1_u32 out_cap,
                            woff1_u32 *out_len,
                            woff1_report *report);
const char *woff1_error_string(int code);
void woff1_tag_to_cstr(woff1_u32 tag, char out5[5]);

#ifdef __cplusplus
}
#endif

#endif
