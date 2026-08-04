#include "sff_stdio_adapter.h"

static int sff__stdio_read_at(void *user, sff_u32 offset, void *dst, sff_u32 len)
{
    SffStdioSource *src;
    size_t got;

    src = (SffStdioSource*)user;
    if (!src || !src->fp || !dst) return 0;
    if (fseek(src->fp, (long)offset, SEEK_SET) != 0) return 0;
    got = fread(dst, 1u, (size_t)len, src->fp);
    return got == (size_t)len ? 1 : 0;
}

int sff_stdio_source_init(SffStdioSource *src, FILE *fp)
{
    long cur;
    long end;

    if (!src || !fp) return 0;
    src->fp = fp;
    cur = ftell(fp);
    if (cur < 0) return 0;
    if (fseek(fp, 0L, SEEK_END) != 0) return 0;
    end = ftell(fp);
    if (end < 0) return 0;
    if ((unsigned long)end > 0xFFFFFFFFul) return 0;
    if (fseek(fp, cur, SEEK_SET) != 0) return 0;
    src->size = (sff_u32)end;
    return 1;
}

void sff_stdio_make_io(SffStdioSource *src, SffIo *io)
{
    if (!src || !io) return;
    io->read_at = sff__stdio_read_at;
    io->user = src;
    io->size = src->size;
}
