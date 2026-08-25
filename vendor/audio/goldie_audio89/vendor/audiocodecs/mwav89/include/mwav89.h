#ifndef MWAV89_H
#define MWAV89_H

/* mwav89: allocationless RIFF/WAVE memory parser, C89. */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char mwav89_u8;
typedef unsigned short mwav89_u16;
typedef unsigned int mwav89_u32;

typedef char mwav89_u8_must_be_1[(sizeof(mwav89_u8) == 1) ? 1 : -1];
typedef char mwav89_u16_must_be_2[(sizeof(mwav89_u16) == 2) ? 1 : -1];
typedef char mwav89_u32_must_be_4[(sizeof(mwav89_u32) == 4) ? 1 : -1];

enum mwav89_result {
    MWAV89_OK = 0,
    MWAV89_ERR_ARGUMENT = -1,
    MWAV89_ERR_TOO_SMALL = -2,
    MWAV89_ERR_NOT_RIFF = -3,
    MWAV89_ERR_NOT_WAVE = -4,
    MWAV89_ERR_TRUNCATED = -5,
    MWAV89_ERR_NO_FMT = -6,
    MWAV89_ERR_NO_DATA = -7,
    MWAV89_ERR_UNSUPPORTED_FORMAT = -8,
    MWAV89_ERR_BAD_FMT = -9
};

typedef struct mwav89_view {
    mwav89_u16 format_tag;
    mwav89_u16 channels;
    mwav89_u32 sample_rate;
    mwav89_u32 byte_rate;
    mwav89_u16 block_align;
    mwav89_u16 bits_per_sample;
    const mwav89_u8 *pcm_data;
    mwav89_u32 pcm_bytes;
} mwav89_view;

int mwav89_parse_memory(
    const void *memory,
    mwav89_u32 memory_bytes,
    mwav89_view *out_view
);

const char *mwav89_result_string(int result);

#ifdef __cplusplus
}
#endif

#endif
