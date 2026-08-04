#include "span/span.h"

ddsl_span ddsl_span_make(int line, int col, int offset, int len) {
    ddsl_span s;
    s.line = line;
    s.col = col;
    s.offset = offset;
    s.len = len;
    return s;
}

int ddsl_span_is_valid(ddsl_span s) {
    return (s.line >= 0 && s.col >= 0 && s.offset >= 0 && s.len >= 0) ? 1 : 0;
}
