#ifndef PSD89_STDIO_H
#define PSD89_STDIO_H

#include <stdio.h>

#include "psd89.h"

#ifdef __cplusplus
extern "C" {
#endif

void psd89_stdio_make_io(FILE *fp, psd89_io *io);
int psd89_read_file(psd89_doc *doc, FILE *fp);
int psd89_write_file(FILE *fp, const psd89_doc *doc);
int psd89_read_path(psd89_doc *doc, const char *path);
int psd89_write_path(const char *path, const psd89_doc *doc);

#ifdef __cplusplus
}
#endif

#endif
