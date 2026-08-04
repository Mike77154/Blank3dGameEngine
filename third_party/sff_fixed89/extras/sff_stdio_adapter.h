#ifndef SFF_STDIO_ADAPTER_H
#define SFF_STDIO_ADAPTER_H

#include "../sff/sff_api.h"
#include <stdio.h>

typedef struct SffStdioSource {
    FILE *fp;
    sff_u32 size;
} SffStdioSource;

int sff_stdio_source_init(SffStdioSource *src, FILE *fp);
void sff_stdio_make_io(SffStdioSource *src, SffIo *io);

#endif /* SFF_STDIO_ADAPTER_H */
