#include "stream/stream.h"

#include <string.h>

static void stream_copy(char *dst, int cap, const char *src) {
    int n;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    n = (int)strlen(src);
    if (n > cap - 1) n = cap - 1;
    if (n > 0) memcpy(dst, src, (size_t)n);
    dst[n] = '\0';
}

void ddsl_stream_init(ddsl_stream *s, const char *name, const char *data, int len) {
    if (!s) return;
    s->data = data ? data : "";
    if (len < 0) len = (int)strlen(s->data);
    s->len = len;
    s->pos = 0;
    stream_copy(s->name, (int)sizeof(s->name), name);
}

int ddsl_stream_peek(const ddsl_stream *s) {
    if (!s || !s->data || s->pos < 0 || s->pos >= s->len) return -1;
    return (unsigned char)s->data[s->pos];
}

int ddsl_stream_next(ddsl_stream *s) {
    int c;
    c = ddsl_stream_peek(s);
    if (c >= 0) s->pos++;
    return c;
}

int ddsl_stream_done(const ddsl_stream *s) {
    if (!s || !s->data) return 1;
    return (s->pos >= s->len) ? 1 : 0;
}
