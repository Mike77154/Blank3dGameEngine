#include "vp8_filter.h"

#include "../utils/fixed.h"

static int GWPVP8Abs(int v) {
  return v < 0 ? -v : v;
}

static int GWPVP8ClampSigned8(int v) {
  if (v < -128) return -128;
  if (v > 127) return 127;
  return v;
}

static int GWPVP8U2S(GWPu8 v) {
  return (int)v - 128;
}

static GWPu8 GWPVP8S2U(int v) {
  return GWPClamp8(v + 128);
}

static int GWPVP8Hev(int hev_threshold, int p1, int p0, int q0, int q1) {
  return (GWPVP8Abs(p1 - p0) > hev_threshold ||
          GWPVP8Abs(q1 - q0) > hev_threshold) ? 1 : 0;
}

static int GWPVP8FilterYes(int interior_limit,
                           int edge_limit,
                           int q3,
                           int q2,
                           int q1,
                           int q0,
                           int p0,
                           int p1,
                           int p2,
                           int p3) {
  return ((GWPVP8Abs(p0 - q0) * 2 + (GWPVP8Abs(p1 - q1) >> 1)) <= edge_limit &&
          GWPVP8Abs(p3 - p2) <= interior_limit &&
          GWPVP8Abs(p2 - p1) <= interior_limit &&
          GWPVP8Abs(p1 - p0) <= interior_limit &&
          GWPVP8Abs(q3 - q2) <= interior_limit &&
          GWPVP8Abs(q2 - q1) <= interior_limit &&
          GWPVP8Abs(q1 - q0) <= interior_limit);
}

static int GWPVP8CommonAdjust(int use_outer_taps,
                              GWPu8* p1,
                              GWPu8* p0,
                              GWPu8* q0,
                              GWPu8* q1) {
  int P1;
  int P0;
  int Q0;
  int Q1;
  int a;
  int b;
  P1 = GWPVP8U2S(*p1);
  P0 = GWPVP8U2S(*p0);
  Q0 = GWPVP8U2S(*q0);
  Q1 = GWPVP8U2S(*q1);
  a = GWPVP8ClampSigned8((use_outer_taps ? GWPVP8ClampSigned8(P1 - Q1) : 0) + 3 * (Q0 - P0));
  b = GWPVP8ClampSigned8(a + 3) >> 3;
  a = GWPVP8ClampSigned8(a + 4) >> 3;
  *q0 = GWPVP8S2U(Q0 - a);
  *p0 = GWPVP8S2U(P0 + b);
  return a;
}

static void GWPVP8SimpleFilterOne(GWPu8* ptr, int step, int edge_limit) {
  GWPu8* p1;
  GWPu8* p0;
  GWPu8* q0;
  GWPu8* q1;
  p1 = ptr - 2 * step;
  p0 = ptr - step;
  q0 = ptr;
  q1 = ptr + step;
  if ((GWPVP8Abs((int)*p0 - (int)*q0) * 2 + (GWPVP8Abs((int)*p1 - (int)*q1) >> 1)) <= edge_limit) {
    (void)GWPVP8CommonAdjust(1, p1, p0, q0, q1);
  }
}

static void GWPVP8SubblockFilterOne(GWPu8* ptr,
                                    int step,
                                    int edge_limit,
                                    int interior_limit,
                                    int hev_threshold) {
  GWPu8* p3;
  GWPu8* p2;
  GWPu8* p1;
  GWPu8* p0;
  GWPu8* q0;
  GWPu8* q1;
  GWPu8* q2;
  GWPu8* q3;
  int P3;
  int P2;
  int P1;
  int P0;
  int Q0;
  int Q1;
  int Q2;
  int Q3;
  int hv;
  int a;
  p3 = ptr - 4 * step;
  p2 = ptr - 3 * step;
  p1 = ptr - 2 * step;
  p0 = ptr - step;
  q0 = ptr;
  q1 = ptr + step;
  q2 = ptr + 2 * step;
  q3 = ptr + 3 * step;
  P3 = GWPVP8U2S(*p3);
  P2 = GWPVP8U2S(*p2);
  P1 = GWPVP8U2S(*p1);
  P0 = GWPVP8U2S(*p0);
  Q0 = GWPVP8U2S(*q0);
  Q1 = GWPVP8U2S(*q1);
  Q2 = GWPVP8U2S(*q2);
  Q3 = GWPVP8U2S(*q3);
  if (!GWPVP8FilterYes(interior_limit, edge_limit, Q3, Q2, Q1, Q0, P0, P1, P2, P3)) return;
  hv = GWPVP8Hev(hev_threshold, P1, P0, Q0, Q1);
  a = (GWPVP8CommonAdjust(hv, p1, p0, q0, q1) + 1) >> 1;
  if (!hv) {
    *q1 = GWPVP8S2U(Q1 - a);
    *p1 = GWPVP8S2U(P1 + a);
  }
}

static void GWPVP8MBFilterOne(GWPu8* ptr,
                              int step,
                              int edge_limit,
                              int interior_limit,
                              int hev_threshold) {
  GWPu8* p3;
  GWPu8* p2;
  GWPu8* p1;
  GWPu8* p0;
  GWPu8* q0;
  GWPu8* q1;
  GWPu8* q2;
  GWPu8* q3;
  int P3;
  int P2;
  int P1;
  int P0;
  int Q0;
  int Q1;
  int Q2;
  int Q3;
  int w;
  int a;
  p3 = ptr - 4 * step;
  p2 = ptr - 3 * step;
  p1 = ptr - 2 * step;
  p0 = ptr - step;
  q0 = ptr;
  q1 = ptr + step;
  q2 = ptr + 2 * step;
  q3 = ptr + 3 * step;
  P3 = GWPVP8U2S(*p3);
  P2 = GWPVP8U2S(*p2);
  P1 = GWPVP8U2S(*p1);
  P0 = GWPVP8U2S(*p0);
  Q0 = GWPVP8U2S(*q0);
  Q1 = GWPVP8U2S(*q1);
  Q2 = GWPVP8U2S(*q2);
  Q3 = GWPVP8U2S(*q3);
  if (!GWPVP8FilterYes(interior_limit, edge_limit, Q3, Q2, Q1, Q0, P0, P1, P2, P3)) return;
  if (!GWPVP8Hev(hev_threshold, P1, P0, Q0, Q1)) {
    w = GWPVP8ClampSigned8(GWPVP8ClampSigned8(P1 - Q1) + 3 * (Q0 - P0));
    a = GWPVP8ClampSigned8((27 * w + 63) >> 7);
    *q0 = GWPVP8S2U(Q0 - a);
    *p0 = GWPVP8S2U(P0 + a);
    a = GWPVP8ClampSigned8((18 * w + 63) >> 7);
    *q1 = GWPVP8S2U(Q1 - a);
    *p1 = GWPVP8S2U(P1 + a);
    a = GWPVP8ClampSigned8((9 * w + 63) >> 7);
    *q2 = GWPVP8S2U(Q2 - a);
    *p2 = GWPVP8S2U(P2 + a);
  } else {
    (void)GWPVP8CommonAdjust(1, p1, p0, q0, q1);
  }
}

int GWPVP8LoopFilterInteriorLimit(int filter_level, int sharpness_level) {
  int interior_limit;
  interior_limit = filter_level;
  if (sharpness_level) {
    interior_limit >>= (sharpness_level > 4) ? 2 : 1;
    if (interior_limit > 9 - sharpness_level) {
      interior_limit = 9 - sharpness_level;
    }
  }
  if (interior_limit < 1) interior_limit = 1;
  return interior_limit;
}

int GWPVP8LoopFilterHevThreshold(int filter_level, GWPBool key_frame) {
  int hev_threshold;
  hev_threshold = 0;
  if (key_frame) {
    if (filter_level >= 40) hev_threshold = 2;
    else if (filter_level >= 15) hev_threshold = 1;
  } else {
    if (filter_level >= 40) hev_threshold = 3;
    else if (filter_level >= 20) hev_threshold = 2;
    else if (filter_level >= 15) hev_threshold = 1;
  }
  return hev_threshold;
}

int GWPVP8LoopFilterMBEdgeLimit(int filter_level, int interior_limit) {
  int limit;
  limit = ((filter_level + 2) * 2) + interior_limit;
  if (limit < 1) limit = 1;
  return limit;
}

int GWPVP8LoopFilterSubblockEdgeLimit(int filter_level, int interior_limit) {
  int limit;
  limit = (filter_level * 2) + interior_limit;
  if (limit < 1) limit = 1;
  return limit;
}

void GWPVP8SimpleFilterVerticalEdge(GWPu8* ptr,
                                    GWPu32 stride,
                                    int edge_limit,
                                    int count) {
  int i;
  if (ptr == 0 || count <= 0 || edge_limit <= 0) return;
  for (i = 0; i < count; ++i) {
    GWPVP8SimpleFilterOne(ptr + i * stride, 1, edge_limit);
  }
}

void GWPVP8SimpleFilterHorizontalEdge(GWPu8* ptr,
                                      GWPu32 stride,
                                      int edge_limit,
                                      int count) {
  int i;
  if (ptr == 0 || count <= 0 || edge_limit <= 0) return;
  for (i = 0; i < count; ++i) {
    GWPVP8SimpleFilterOne(ptr + i, (int)stride, edge_limit);
  }
}

void GWPVP8NormalMBFilterVerticalEdge(GWPu8* ptr,
                                      GWPu32 stride,
                                      int edge_limit,
                                      int interior_limit,
                                      int hev_threshold,
                                      int count) {
  int i;
  if (ptr == 0 || count <= 0 || edge_limit <= 0) return;
  for (i = 0; i < count; ++i) {
    GWPVP8MBFilterOne(ptr + i * stride, 1, edge_limit, interior_limit, hev_threshold);
  }
}

void GWPVP8NormalMBFilterHorizontalEdge(GWPu8* ptr,
                                        GWPu32 stride,
                                        int edge_limit,
                                        int interior_limit,
                                        int hev_threshold,
                                        int count) {
  int i;
  if (ptr == 0 || count <= 0 || edge_limit <= 0) return;
  for (i = 0; i < count; ++i) {
    GWPVP8MBFilterOne(ptr + i, (int)stride, edge_limit, interior_limit, hev_threshold);
  }
}

void GWPVP8NormalSubblockFilterVerticalEdge(GWPu8* ptr,
                                            GWPu32 stride,
                                            int edge_limit,
                                            int interior_limit,
                                            int hev_threshold,
                                            int count) {
  int i;
  if (ptr == 0 || count <= 0 || edge_limit <= 0) return;
  for (i = 0; i < count; ++i) {
    GWPVP8SubblockFilterOne(ptr + i * stride, 1, edge_limit, interior_limit, hev_threshold);
  }
}

void GWPVP8NormalSubblockFilterHorizontalEdge(GWPu8* ptr,
                                              GWPu32 stride,
                                              int edge_limit,
                                              int interior_limit,
                                              int hev_threshold,
                                              int count) {
  int i;
  if (ptr == 0 || count <= 0 || edge_limit <= 0) return;
  for (i = 0; i < count; ++i) {
    GWPVP8SubblockFilterOne(ptr + i, (int)stride, edge_limit, interior_limit, hev_threshold);
  }
}
