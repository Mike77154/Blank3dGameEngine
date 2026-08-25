#ifndef BMP_REPORT_H
#define BMP_REPORT_H

#include "../include/bmp/bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "bmp_parser.h"

#define BMP_VERSION_MAJOR 1
#define BMP_VERSION_MINOR 16
#define BMP_VERSION_PATCH 0
#define BMP_VERSION_STRING "1.16.0"
#define BMP_VERSION_NUMBER ((bmp_u32)(BMP_VERSION_MAJOR * 10000U + BMP_VERSION_MINOR * 100U + BMP_VERSION_PATCH))
#define BMP_REPORT_STACK_CAP 1024U

BMP_EXPORT const char *bmp_version_string(void);
BMP_EXPORT bmp_u32 bmp_version_number(void);
BMP_EXPORT const char *bmp_dib_type_string(enum bmp_dib_type type);
BMP_EXPORT const char *bmp_compression_string(bmp_u32 compression);
BMP_EXPORT int bmp_warning_count(bmp_u32 mask);

BMP_EXPORT int bmp_format_diagnostics_text(const bmp_image *img,
                                char *buffer,
                                bmp_u32 buffer_size);
BMP_EXPORT int bmp_format_diagnostics_json(const bmp_image *img,
                                char *buffer,
                                bmp_u32 buffer_size);
BMP_EXPORT int bmp_write_diagnostics_text(FILE *f, const bmp_image *img);
BMP_EXPORT int bmp_write_diagnostics_json(FILE *f, const bmp_image *img);

#ifdef __cplusplus
}
#endif

#endif
