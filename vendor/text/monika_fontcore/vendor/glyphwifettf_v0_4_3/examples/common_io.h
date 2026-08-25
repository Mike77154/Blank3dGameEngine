#ifndef GWT_COMMON_IO_H
#define GWT_COMMON_IO_H
#include <stdio.h>
#include <stdlib.h>

#define FONT_BUF_MAX (32UL*1024UL*1024UL)
static unsigned char g_font_buf[FONT_BUF_MAX];

static unsigned long read_file_static(const char *path, unsigned char *buf, unsigned long max_size){
    FILE *fp; unsigned long n;
    fp = fopen(path, "rb");
    if(!fp) return 0;
    n = (unsigned long)fread(buf, 1, max_size, fp);
    fclose(fp);
    return n;
}

#endif
