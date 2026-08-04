#ifndef ECG_PNG_WRITER_H
#define ECG_PNG_WRITER_H

#include "ecg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ECG_Status ecg_png_write_rgb24(const char *filename, const ECG_Surface *surface);

#ifdef __cplusplus
}
#endif

#endif
