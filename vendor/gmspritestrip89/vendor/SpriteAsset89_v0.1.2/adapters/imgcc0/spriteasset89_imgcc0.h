#ifndef SPRITEASSET89_IMGCC0_H
#define SPRITEASSET89_IMGCC0_H
#include "spriteasset89.h"
#include "imgcc0.h"

#ifndef SA89_IMGCC0_MAX_HANDLES
#define SA89_IMGCC0_MAX_HANDLES SA89_MAX_SOURCES
#endif

typedef struct SA89_Imgcc0Entry_s {
    char path[SA89_PATH_CAP];
    sa89_u32 width;
    sa89_u32 height;
    sa89_u32 frames;
    unsigned char used;
} SA89_Imgcc0Entry;

typedef struct SA89_Imgcc0Adapter_s {
    void *output_buffer;
    imgcc0_u32 output_size;
    void *temp_buffer;
    imgcc0_u32 temp_size;
    void *file_buffer;
    imgcc0_u32 file_size;
    imgcc0_image image;
    SA89_Imgcc0Entry entries[SA89_IMGCC0_MAX_HANDLES];
    sa89_u32 current_handle;
    sa89_u32 entry_count;
    int loaded;
} SA89_Imgcc0Adapter;

void sa89_imgcc0_init(SA89_Imgcc0Adapter *ctx,
                      void *output_buffer, imgcc0_u32 output_size,
                      void *temp_buffer, imgcc0_u32 temp_size,
                      void *file_buffer, imgcc0_u32 file_size);
void sa89_imgcc0_make_provider(SA89_Imgcc0Adapter *ctx, SA89_ImageProvider *out_provider);
#endif
