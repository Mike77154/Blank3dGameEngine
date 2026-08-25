#ifndef SPRITEASSET89_IMGCC0_H
#define SPRITEASSET89_IMGCC0_H
#include "spriteasset89.h"
#include "imgcc0.h"

typedef struct SA89_Imgcc0Adapter_s {
    void *output_buffer;
    imgcc0_u32 output_size;
    void *temp_buffer;
    imgcc0_u32 temp_size;
    void *file_buffer;
    imgcc0_u32 file_size;
    imgcc0_image image;
    char current_path[SA89_PATH_CAP];
    int loaded;
} SA89_Imgcc0Adapter;

void sa89_imgcc0_init(SA89_Imgcc0Adapter *ctx,
                      void *output_buffer, imgcc0_u32 output_size,
                      void *temp_buffer, imgcc0_u32 temp_size,
                      void *file_buffer, imgcc0_u32 file_size);
void sa89_imgcc0_make_provider(SA89_Imgcc0Adapter *ctx, SA89_ImageProvider *out_provider);
#endif
