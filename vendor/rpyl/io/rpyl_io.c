#include "rpyl_io.h"
#include "rpyl_common.h"

#ifndef EOF
#define EOF (-1)
#endif

void rpyl_io_buffer_init(RpylIoBuffer* b, const char* data, size_t size) {
    if (b != 0) {
        b->data = data ? data : "";
        b->size = size;
        b->pos = 0u;
    }
}

size_t rpyl_io_buffer_read(RpylIoBuffer* b, void* dst, size_t bytes) {
    size_t left;
    unsigned char* d;
    size_t i;
    if (b == 0 || dst == 0 || b->data == 0) return 0u;
    if (b->pos >= b->size) return 0u;
    left = b->size - b->pos;
    if (bytes > left) bytes = left;
    d = (unsigned char*)dst;
    for (i = 0u; i < bytes; i++) d[i] = (unsigned char)b->data[b->pos + i];
    b->pos += bytes;
    return bytes;
}

int rpyl_io_buffer_getc(void* user) {
    RpylIoBuffer* b;
    b = (RpylIoBuffer*)user;
    if (!b || !b->data) return EOF;
    if (b->pos >= b->size) return EOF;
    return (int)(unsigned char)b->data[b->pos++];
}

int rpyl_io_buffer_ungetc(int c, void* user) {
    RpylIoBuffer* b;
    b = (RpylIoBuffer*)user;
    if (!b || c == EOF) return EOF;
    if (b->pos == 0u) return EOF;
    b->pos--;
    return c;
}

int rpyl_io_buffer_stream(RpylIoBuffer* b, RpylStream* out_stream) {
    if (!b || !out_stream) return 0;
    out_stream->user = b;
    out_stream->getc_fn = rpyl_io_buffer_getc;
    out_stream->ungetc_fn = rpyl_io_buffer_ungetc;
    return 1;
}

void rpyl_io_source_init(RpylIoSource* src, void* user, RpylIoGetcFn getc_fn, RpylIoUngetcFn ungetc_fn) {
    if (!src) return;
    src->user = user;
    src->getc_fn = getc_fn;
    src->ungetc_fn = ungetc_fn;
    src->line = 1UL;
    src->column = 0UL;
    src->offset = 0UL;
    src->last_char = 0;
}

int rpyl_io_source_getc(RpylIoSource* src) {
    int c;
    if (!src || !src->getc_fn) return EOF;
    c = src->getc_fn(src->user);
    if (c == EOF) return EOF;
    src->offset++;
    if (c == '\n') {
        src->line++;
        src->column = 0UL;
    } else {
        src->column++;
    }
    src->last_char = c;
    return c;
}

int rpyl_io_source_ungetc(RpylIoSource* src, int c) {
    if (!src || !src->ungetc_fn || c == EOF) return EOF;
    if (src->ungetc_fn(c, src->user) == EOF) return EOF;
    if (src->offset > 0UL) src->offset--;
    if (c == '\n') {
        if (src->line > 1UL) src->line--;
        src->column = 0UL;
    } else if (src->column > 0UL) {
        src->column--;
    }
    return c;
}

int rpyl_io_source_peek(RpylIoSource* src) {
    int c;
    c = rpyl_io_source_getc(src);
    if (c != EOF) (void)rpyl_io_source_ungetc(src, c);
    return c;
}

size_t rpyl_io_source_read_line(RpylIoSource* src, char* out, size_t out_size) {
    size_t n;
    int c;
    if (!src || !out || out_size == 0u) return 0u;
    out[0] = '\0';
    n = 0u;
    while (n + 1u < out_size) {
        c = rpyl_io_source_getc(src);
        if (c == EOF) break;
        out[n++] = (char)c;
        if (c == '\n') break;
    }
    out[n] = '\0';
    return n;
}

static int source_getc_adapter(void* user) {
    return rpyl_io_source_getc((RpylIoSource*)user);
}

static int source_ungetc_adapter(int c, void* user) {
    return rpyl_io_source_ungetc((RpylIoSource*)user, c);
}

int rpyl_io_source_stream(RpylIoSource* src, RpylStream* out_stream) {
    if (!src || !out_stream) return 0;
    out_stream->user = src;
    out_stream->getc_fn = source_getc_adapter;
    out_stream->ungetc_fn = source_ungetc_adapter;
    return 1;
}

void rpyl_io_read_adapter_init(RpylIoReadAdapter* a, void* user, void* handle, RpylIoReadFn read_fn) {
    if (!a) return;
    a->user = user;
    a->handle = handle;
    a->read_fn = read_fn;
    a->pos = 0u;
    a->len = 0u;
    a->has_push = 0;
    a->push = 0;
}

static int read_adapter_fill(RpylIoReadAdapter* a) {
    if (!a || !a->read_fn) return 0;
    a->pos = 0u;
    a->len = a->read_fn(a->user, a->handle, a->buf, sizeof(a->buf));
    return a->len > 0u ? 1 : 0;
}

int rpyl_io_read_adapter_getc(void* user) {
    RpylIoReadAdapter* a;
    a = (RpylIoReadAdapter*)user;
    if (!a) return EOF;
    if (a->has_push) {
        a->has_push = 0;
        return a->push;
    }
    if (a->pos >= a->len) {
        if (!read_adapter_fill(a)) return EOF;
    }
    return (int)a->buf[a->pos++];
}

int rpyl_io_read_adapter_ungetc(int c, void* user) {
    RpylIoReadAdapter* a;
    a = (RpylIoReadAdapter*)user;
    if (!a || c == EOF || a->has_push) return EOF;
    a->has_push = 1;
    a->push = c;
    return c;
}
