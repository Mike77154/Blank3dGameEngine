#include "bmp_chunks.h"

void bmp_stream_init(bmp_stream *s, const bmp_u8 *data, bmp_u32 size)
{
    if (!s) return;
    s->data  = data;
    s->size  = size;
    s->pos   = 0;
    s->error = 0;
}

static int bmp_stream_ensure(bmp_stream *s, bmp_u32 count)
{
    if (!s || s->error) return 0;
    if (count > s->size || s->pos > s->size - count) {
        s->error = 1;
        return 0;
    }
    return 1;
}

bmp_u8 bmp_read_u8(bmp_stream *s)
{
    if (!bmp_stream_ensure(s, 1)) return 0;
    return s->data[s->pos++];
}

bmp_u16 bmp_read_u16(bmp_stream *s)
{
    bmp_u16 v;
    bmp_u8 b0, b1;
    if (!bmp_stream_ensure(s, 2)) return 0;
    b0 = s->data[s->pos++];
    b1 = s->data[s->pos++];
    v = (bmp_u16)(b0 | ((bmp_u16)b1 << 8));
    return v;
}

bmp_u32 bmp_read_u32(bmp_stream *s)
{
    bmp_u32 v;
    bmp_u8 b0, b1, b2, b3;
    if (!bmp_stream_ensure(s, 4)) return 0;
    b0 = s->data[s->pos++];
    b1 = s->data[s->pos++];
    b2 = s->data[s->pos++];
    b3 = s->data[s->pos++];
    v = (bmp_u32)b0 |
        ((bmp_u32)b1 << 8) |
        ((bmp_u32)b2 << 16) |
        ((bmp_u32)b3 << 24);
    return v;
}

bmp_s32 bmp_read_s32(bmp_stream *s)
{
    return (bmp_s32)bmp_read_u32(s);
}

void bmp_stream_skip(bmp_stream *s, bmp_u32 count)
{
    if (!bmp_stream_ensure(s, count)) return;
    s->pos += count;
}

void bmp_stream_seek(bmp_stream *s, bmp_u32 pos)
{
    if (!s) return;
    if (pos > s->size) {
        s->error = 1;
        return;
    }
    s->pos = pos;
}

const bmp_u8 *bmp_stream_peek(bmp_stream *s, bmp_u32 count)
{
    if (!bmp_stream_ensure(s, count)) return 0;
    return s->data + s->pos;
}

bmp_u32 bmp_stream_remaining(const bmp_stream *s)
{
    if (!s || s->pos > s->size) return 0;
    return s->size - s->pos;
}

int bmp_stream_failed(const bmp_stream *s)
{
    return s ? s->error : 1;
}
