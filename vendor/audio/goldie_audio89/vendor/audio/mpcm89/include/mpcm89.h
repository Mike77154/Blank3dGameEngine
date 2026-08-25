#ifndef MPCM89_H
#define MPCM89_H

/* mpcm89: validated, allocationless integer PCM view, C89. */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char mpcm89_u8;
typedef unsigned short mpcm89_u16;
typedef unsigned int mpcm89_u32;
typedef signed short mpcm89_s16;

typedef char mpcm89_u32_must_be_4[(sizeof(mpcm89_u32) == 4) ? 1 : -1];
typedef char mpcm89_s16_must_be_2[(sizeof(mpcm89_s16) == 2) ? 1 : -1];

enum mpcm89_result {
    MPCM89_OK = 0,
    MPCM89_ERR_ARGUMENT = -1,
    MPCM89_ERR_CHANNELS = -2,
    MPCM89_ERR_SAMPLE_RATE = -3,
    MPCM89_ERR_BITS = -4,
    MPCM89_ERR_BLOCK_ALIGN = -5,
    MPCM89_ERR_BYTE_RATE = -6,
    MPCM89_ERR_DATA_SIZE = -7
};

typedef struct mpcm89_format {
    mpcm89_u16 channels;
    mpcm89_u32 sample_rate;
    mpcm89_u32 byte_rate;
    mpcm89_u16 block_align;
    mpcm89_u16 bits_per_sample;
} mpcm89_format;

typedef struct mpcm89_clip {
    mpcm89_format format;
    const mpcm89_u8 *bytes;
    mpcm89_u32 byte_count;
    mpcm89_u32 frame_count;
} mpcm89_clip;

int mpcm89_clip_init(
    mpcm89_clip *clip,
    const mpcm89_u8 *bytes,
    mpcm89_u32 byte_count,
    const mpcm89_format *format
);

mpcm89_u32 mpcm89_duration_ms(const mpcm89_clip *clip);
const char *mpcm89_result_string(int result);

#ifdef __cplusplus
}
#endif

#endif
