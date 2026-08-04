#ifndef RPYL_SPAN_H
#define RPYL_SPAN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpylSpan {
    unsigned long file_id;
    unsigned long line;
    unsigned long column;
    unsigned long offset;
    unsigned long length;
} RpylSpan;

RpylSpan rpyl_span_make(unsigned long file_id, unsigned long line, unsigned long column, unsigned long offset, unsigned long length);
RpylSpan rpyl_span_zero(void);
int rpyl_span_is_valid(RpylSpan span);
unsigned long rpyl_span_end_offset(RpylSpan span);
int rpyl_span_contains_offset(RpylSpan span, unsigned long offset);
int rpyl_span_same_line(RpylSpan a, RpylSpan b);
RpylSpan rpyl_span_join(RpylSpan a, RpylSpan b);

#ifdef __cplusplus
}
#endif

#endif
