#ifndef GF_FIXED_H
#define GF_FIXED_H

#define GF_FP_SHIFT 6
#define GF_FP_ONE   64L
#define GF_FP_HALF  32L
#define GF_FX(x)    ((long)((x) << GF_FP_SHIFT))
#define GF_INT(x)   ((int)((x) >> GF_FP_SHIFT))
#define GF_ABS(x)   ((x) < 0 ? -(x) : (x))
#define GF_MIN(a,b) ((a) < (b) ? (a) : (b))
#define GF_MAX(a,b) ((a) > (b) ? (a) : (b))

static long gf_mul(long a, long b) { return (long)((a * b) >> GF_FP_SHIFT); }
static long gf_div(long a, long b) { if (b == 0) return 0; return (long)((a << GF_FP_SHIFT) / b); }
static long gf_lerp(long a, long b, long t) { return a + gf_mul((b - a), t); }

#endif
