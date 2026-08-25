#ifndef ZRAGF_PROTOCOL89_STDIO_H_INCLUDED
#define ZRAGF_PROTOCOL89_STDIO_H_INCLUDED

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "zragflib.h"

#ifndef ZRAGF_P89_FORMAT_BYTES
#define ZRAGF_P89_FORMAT_BYTES 4096u
#endif

static int zragf_p89_format(char *dst, zragf_size_t cap, const char *fmt, ...)
{
    static char temp[ZRAGF_P89_FORMAT_BYTES];
    va_list ap;
    int count;
    zragf_size_t copy_n;
    if (!dst || cap == 0u || !fmt)
        return -1;
    va_start(ap, fmt);
    count = vsprintf(temp, fmt, ap);
    va_end(ap);
    if (count < 0) {
        dst[0] = '\0';
        return count;
    }
    copy_n = (zragf_size_t)count;
    if (copy_n >= cap)
        copy_n = cap - 1u;
    if (copy_n > 0u)
        memcpy(dst, temp, copy_n);
    dst[copy_n] = '\0';
    return count;
}

#endif
