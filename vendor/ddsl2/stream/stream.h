#ifndef DDSL_STREAM_H
#define DDSL_STREAM_H

#include "config/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ddsl_stream {
    const char *data;
    int len;
    int pos;
    char name[DDSL_MAX_STREAM_NAME];
} ddsl_stream;

void ddsl_stream_init(ddsl_stream *s, const char *name, const char *data, int len);
int ddsl_stream_peek(const ddsl_stream *s);
int ddsl_stream_next(ddsl_stream *s);
int ddsl_stream_done(const ddsl_stream *s);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_STREAM_H */
