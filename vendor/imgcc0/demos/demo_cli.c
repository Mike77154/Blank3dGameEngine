#include "imgcc0.h"

#include <stdio.h>
#include <string.h>

#define DEMO_OUTPUT_BYTES (16U * 1024U * 1024U)
#define DEMO_TEMP_BYTES   (16U * 1024U * 1024U)
#define DEMO_FILE_BYTES   (8U * 1024U * 1024U)

IMGCC0_DECLARE_BUFFER(g_output, DEMO_OUTPUT_BYTES);
IMGCC0_DECLARE_BUFFER(g_temp, DEMO_TEMP_BYTES);
IMGCC0_DECLARE_BUFFER(g_file, DEMO_FILE_BYTES);

static int write_pam_rgba(const char *path, const imgcc0_frame *frame)
{
    FILE *fp;
    const char *hdr1;
    const char *hdr2;
    imgcc0_u32 bytes;
    hdr1 = "P7\n";
    hdr2 = "DEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n";
    if (path == 0 || frame == 0 || frame->pixels == 0) return 0;
    fp = fopen(path, "wb");
    if (fp == 0) return 0;
    fputs(hdr1, fp);
    fprintf(fp, "WIDTH %u\nHEIGHT %u\n", frame->width, frame->height);
    fputs(hdr2, fp);
    bytes = frame->stride * frame->height;
    if (bytes != 0U) {
        if ((imgcc0_u32)fwrite(frame->pixels, 1U, bytes, fp) != bytes) {
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return 1;
}

static void print_info(const char *path, const imgcc0_image *img)
{
    imgcc0_u32 i;
    printf("file=%s\n", path ? path : "(null)");
    if (img == 0) {
        printf("ok=0\nerror=invalid image object\n");
        return;
    }
    printf("ok=%d\n", img->ok);
    printf("format=%s\n", imgcc0_format_name(img->format));
    printf("width=%u\n", img->width);
    printf("height=%u\n", img->height);
    printf("animated=%d\n", img->is_animated);
    printf("frames=%u\n", img->frame_count);
    printf("loop_count=%d\n", img->loop_count);
    printf("output_used=%u\n", img->output_used);
    printf("temp_peak=%u\n", img->temp_peak);
    printf("error_code=%d\n", img->error_code);
    printf("error_message=%s\n", img->error_message);
    for (i = 0U; i < img->frame_count; ++i) {
        printf("frame[%u].delay_ms=%u\n", i, img->frames[i].delay_ms);
    }
}

int main(int argc, char **argv)
{
    imgcc0_image img;
    imgcc0_open_options opt;
    int rc;
    int i;
    int dump_index;
    if (argc < 2) {
        fprintf(stderr, "usage: %s <image-file> [--dump-prefix PREFIX]\n", argv[0]);
        return 2;
    }
    dump_index = -1;
    for (i = 2; i + 1 < argc; ++i) {
        if (strcmp(argv[i], "--dump-prefix") == 0) {
            dump_index = i + 1;
            break;
        }
    }
    imgcc0_image_init(&img);
    imgcc0_open_options_init(&opt);
    opt.output_buffer = IMGCC0_BUFFER_DATA(g_output);
    opt.output_buffer_size = IMGCC0_BUFFER_SIZE(g_output);
    opt.temp_buffer = IMGCC0_BUFFER_DATA(g_temp);
    opt.temp_buffer_size = IMGCC0_BUFFER_SIZE(g_temp);
    opt.file_buffer = IMGCC0_BUFFER_DATA(g_file);
    opt.file_buffer_size = IMGCC0_BUFFER_SIZE(g_file);
    rc = imgcc0_open_file(argv[1], &opt, &img);
    print_info(argv[1], &img);
    if (rc != IMGCC0_OK) return 1;
    if (dump_index > 0) {
        char out_path[1024];
        imgcc0_u32 f;
        for (f = 0U; f < img.frame_count; ++f) {
            sprintf(out_path, "%s_%03u.pam", argv[dump_index], f);
            if (!write_pam_rgba(out_path, &img.frames[f])) {
                fprintf(stderr, "failed to write %s\n", out_path);
                return 1;
            }
            printf("wrote=%s\n", out_path);
        }
    }
    imgcc0_image_reset(&img);
    return 0;
}
