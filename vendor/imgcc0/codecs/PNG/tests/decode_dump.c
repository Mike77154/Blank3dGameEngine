#include <stdio.h>
#include <string.h>
#include "png_decoder.h"

static void put_u32le(FILE* f, png_u32 v)
{
    unsigned char b[4];
    b[0] = (unsigned char)(v & 255u);
    b[1] = (unsigned char)((v >> 8) & 255u);
    b[2] = (unsigned char)((v >> 16) & 255u);
    b[3] = (unsigned char)((v >> 24) & 255u);
    fwrite(b,1,4,f);
}

int main(int argc, char** argv)
{
    png_image img;
    png_decode_options opt;
    FILE* f;
    int err;
    png_u32 total;
    int raw;
    if (argc != 4) return 2;
    raw = strcmp(argv[1], "raw") == 0;
    memset(&img, 0, sizeof(img));
    if (raw) {
        png_decode_options_init(&opt);
        opt.output_format = PNG_OUTPUT_RGBA8;
        opt.transform_flags = PNG_DEC_TRANSFORM_NONE;
        opt.strict_trailing_data = 1;
        err = png_load_file_ex_zlib(argv[2], &opt, &img);
    } else {
        err = png_load_file_zlib(argv[2], &img);
    }
    if (err != PNG_DEC_OK) {
        fprintf(stderr, "ERR=%d\n", err);
        return 10 - err;
    }
    total = img.pixel_rowbytes * img.height;
    f = fopen(argv[3], "wb");
    if (!f) { png_free_image(&img); return 3; }
    put_u32le(f, img.width);
    put_u32le(f, img.height);
    put_u32le(f, img.pixel_rowbytes);
    fwrite(img.pixels,1,total,f);
    fclose(f);
    png_free_image(&img);
    return 0;
}
