#ifndef DDSL_SPAN_H
#define DDSL_SPAN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ddsl_span {
    int line;
    int col;
    int offset;
    int len;
} ddsl_span;

ddsl_span ddsl_span_make(int line, int col, int offset, int len);
int ddsl_span_is_valid(ddsl_span s);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_SPAN_H */
