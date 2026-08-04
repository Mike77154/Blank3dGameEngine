#include "rpyl_span.h"

RpylSpan rpyl_span_make(unsigned long file_id, unsigned long line, unsigned long column, unsigned long offset, unsigned long length) {
    RpylSpan s;
    s.file_id = file_id;
    s.line = line;
    s.column = column;
    s.offset = offset;
    s.length = length;
    return s;
}

RpylSpan rpyl_span_zero(void) {
    RpylSpan s;
    s.file_id = 0UL;
    s.line = 0UL;
    s.column = 0UL;
    s.offset = 0UL;
    s.length = 0UL;
    return s;
}

int rpyl_span_is_valid(RpylSpan span) {
    return span.line != 0UL ? 1 : 0;
}

unsigned long rpyl_span_end_offset(RpylSpan span) {
    unsigned long end_offset;
    end_offset = span.offset + span.length;
    if (end_offset < span.offset) return span.offset;
    return end_offset;
}

int rpyl_span_contains_offset(RpylSpan span, unsigned long offset) {
    unsigned long end_offset;
    if (!rpyl_span_is_valid(span)) return 0;
    end_offset = rpyl_span_end_offset(span);
    if (offset < span.offset) return 0;
    if (offset >= end_offset) return 0;
    return 1;
}

int rpyl_span_same_line(RpylSpan a, RpylSpan b) {
    if (!rpyl_span_is_valid(a) || !rpyl_span_is_valid(b)) return 0;
    if (a.file_id != b.file_id) return 0;
    return a.line == b.line ? 1 : 0;
}

RpylSpan rpyl_span_join(RpylSpan a, RpylSpan b) {
    RpylSpan out;
    unsigned long a_end;
    unsigned long b_end;
    unsigned long start;
    unsigned long end_offset;

    if (!rpyl_span_is_valid(a)) return b;
    if (!rpyl_span_is_valid(b)) return a;
    if (a.file_id != b.file_id) return a;

    start = a.offset < b.offset ? a.offset : b.offset;
    a_end = rpyl_span_end_offset(a);
    b_end = rpyl_span_end_offset(b);
    end_offset = a_end > b_end ? a_end : b_end;

    out = a.offset <= b.offset ? a : b;
    out.offset = start;
    out.length = end_offset >= start ? end_offset - start : 0UL;
    return out;
}
