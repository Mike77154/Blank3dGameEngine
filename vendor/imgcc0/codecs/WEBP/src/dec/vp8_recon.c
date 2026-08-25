#include "vp8_recon.h"

#include "../utils/fixed.h"

static GWPu8 GWPVP8Avg(const GWPu8* a, const GWPu8* b, int n) {
  int i;
  int sum;
  sum = 0;
  for (i = 0; i < n; ++i) sum += a[i] + b[i];
  return GWPClamp8((sum + n) / (2 * n));
}

static GWPu8 GWPVP8Avg2(GWPu8 a, GWPu8 b) {
  return (GWPu8)(((int)a + (int)b + 1) >> 1);
}

static GWPu8 GWPVP8Avg3(GWPu8 a, GWPu8 b, GWPu8 c) {
  return (GWPu8)(((int)a + 2 * (int)b + (int)c + 2) >> 2);
}

void GWPVP8Predict16x16(GWPVP8YMode mode,
                        const GWPu8* above,
                        const GWPu8* left,
                        GWPu8 top_left,
                        GWPu8 out[16 * 16]) {
  int r;
  int c;
  GWPu8 dc;
  if (above == 0 || left == 0 || out == 0) return;
  dc = GWPVP8Avg(above, left, 16);
  for (r = 0; r < 16; ++r) {
    for (c = 0; c < 16; ++c) {
      GWPu8 v;
      v = dc;
      if (mode == GWP_VP8_YMODE_V) {
        v = above[c];
      } else if (mode == GWP_VP8_YMODE_H) {
        v = left[r];
      } else if (mode == GWP_VP8_YMODE_TM) {
        v = GWPClamp8((int)left[r] + (int)above[c] - (int)top_left);
      }
      out[r * 16 + c] = v;
    }
  }
}

void GWPVP8Predict8x8(GWPVP8UVMode mode,
                      const GWPu8* above,
                      const GWPu8* left,
                      GWPu8 top_left,
                      GWPu8 out[8 * 8]) {
  int r;
  int c;
  GWPu8 dc;
  if (above == 0 || left == 0 || out == 0) return;
  dc = GWPVP8Avg(above, left, 8);
  for (r = 0; r < 8; ++r) {
    for (c = 0; c < 8; ++c) {
      GWPu8 v;
      v = dc;
      if (mode == GWP_VP8_UVMODE_V) {
        v = above[c];
      } else if (mode == GWP_VP8_UVMODE_H) {
        v = left[r];
      } else if (mode == GWP_VP8_UVMODE_TM) {
        v = GWPClamp8((int)left[r] + (int)above[c] - (int)top_left);
      }
      out[r * 8 + c] = v;
    }
  }
}


void GWPVP8Predict4x4(GWPVP8BMode mode,
                      const GWPu8* above,
                      const GWPu8* left,
                      GWPu8 top_left,
                      GWPu8 out[4 * 4]) {
  GWPu8 A;
  GWPu8 B;
  GWPu8 C;
  GWPu8 D;
  GWPu8 E;
  GWPu8 F;
  GWPu8 G;
  GWPu8 H;
  GWPu8 J;
  GWPu8 K;
  GWPu8 L;
  GWPu8 M;
  GWPu8 dc;
  int r;
  int c;
  if (above == 0 || left == 0 || out == 0) return;
  A = above[0]; B = above[1]; C = above[2]; D = above[3];
  E = above[4]; F = above[5]; G = above[6]; H = above[7];
  J = left[0]; K = left[1]; L = left[2]; M = left[3];

  switch (mode) {
    case GWP_VP8_BMODE_DC:
      dc = GWPClamp8((((int)A + (int)B + (int)C + (int)D +
                       (int)J + (int)K + (int)L + (int)M + 4) >> 3));
      for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) out[r * 4 + c] = dc;
      }
      break;
    case GWP_VP8_BMODE_TM:
      for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) {
          out[r * 4 + c] = GWPClamp8((int)left[r] + (int)above[c] - (int)top_left);
        }
      }
      break;
    case GWP_VP8_BMODE_VE:
      out[0] = out[4] = out[8] = out[12] = GWPVP8Avg3(top_left, A, B);
      out[1] = out[5] = out[9] = out[13] = GWPVP8Avg3(A, B, C);
      out[2] = out[6] = out[10] = out[14] = GWPVP8Avg3(B, C, D);
      out[3] = out[7] = out[11] = out[15] = GWPVP8Avg3(C, D, E);
      break;
    case GWP_VP8_BMODE_HE:
      for (c = 0; c < 4; ++c) out[c] = GWPVP8Avg3(top_left, J, K);
      for (c = 0; c < 4; ++c) out[4 + c] = GWPVP8Avg3(J, K, L);
      for (c = 0; c < 4; ++c) out[8 + c] = GWPVP8Avg3(K, L, M);
      for (c = 0; c < 4; ++c) out[12 + c] = GWPVP8Avg3(L, M, M);
      break;
    case GWP_VP8_BMODE_LD:
      out[0] = GWPVP8Avg3(A, B, C);
      out[1] = out[4] = GWPVP8Avg3(B, C, D);
      out[2] = out[5] = out[8] = GWPVP8Avg3(C, D, E);
      out[3] = out[6] = out[9] = out[12] = GWPVP8Avg3(D, E, F);
      out[7] = out[10] = out[13] = GWPVP8Avg3(E, F, G);
      out[11] = out[14] = GWPVP8Avg3(F, G, H);
      out[15] = GWPVP8Avg3(G, H, H);
      break;
    case GWP_VP8_BMODE_RD:
      out[12] = GWPVP8Avg3(M, L, K);
      out[13] = out[8] = GWPVP8Avg3(L, K, J);
      out[14] = out[9] = out[4] = GWPVP8Avg3(K, J, top_left);
      out[15] = out[10] = out[5] = out[0] = GWPVP8Avg3(J, top_left, A);
      out[11] = out[6] = out[1] = GWPVP8Avg3(top_left, A, B);
      out[7] = out[2] = GWPVP8Avg3(A, B, C);
      out[3] = GWPVP8Avg3(B, C, D);
      break;
    case GWP_VP8_BMODE_VR:
      out[12] = GWPVP8Avg3(L, K, J);
      out[8] = GWPVP8Avg3(K, J, top_left);
      out[13] = out[4] = GWPVP8Avg3(J, top_left, A);
      out[9] = out[0] = GWPVP8Avg2(top_left, A);
      out[14] = out[5] = GWPVP8Avg3(top_left, A, B);
      out[10] = out[1] = GWPVP8Avg2(A, B);
      out[15] = out[6] = GWPVP8Avg3(A, B, C);
      out[11] = out[2] = GWPVP8Avg2(B, C);
      out[7] = GWPVP8Avg3(B, C, D);
      out[3] = GWPVP8Avg2(C, D);
      break;
    case GWP_VP8_BMODE_VL:
      out[0] = GWPVP8Avg2(A, B);
      out[4] = GWPVP8Avg3(A, B, C);
      out[8] = out[1] = GWPVP8Avg2(B, C);
      out[5] = out[12] = GWPVP8Avg3(B, C, D);
      out[9] = out[2] = GWPVP8Avg2(C, D);
      out[13] = out[6] = GWPVP8Avg3(C, D, E);
      out[10] = out[3] = GWPVP8Avg2(D, E);
      out[14] = out[7] = GWPVP8Avg3(D, E, F);
      out[11] = GWPVP8Avg3(E, F, G);
      out[15] = GWPVP8Avg3(F, G, H);
      break;
    case GWP_VP8_BMODE_HD:
      out[12] = GWPVP8Avg2(M, L);
      out[13] = GWPVP8Avg3(M, L, K);
      out[8] = out[14] = GWPVP8Avg2(L, K);
      out[9] = out[15] = GWPVP8Avg3(L, K, J);
      out[10] = out[4] = GWPVP8Avg2(K, J);
      out[11] = out[5] = GWPVP8Avg3(K, J, top_left);
      out[6] = out[0] = GWPVP8Avg2(J, top_left);
      out[7] = out[1] = GWPVP8Avg3(J, top_left, A);
      out[2] = GWPVP8Avg3(top_left, A, B);
      out[3] = GWPVP8Avg3(A, B, C);
      break;
    case GWP_VP8_BMODE_HU:
    default:
      out[0] = GWPVP8Avg2(J, K);
      out[1] = GWPVP8Avg3(J, K, L);
      out[2] = out[4] = GWPVP8Avg2(K, L);
      out[3] = out[5] = GWPVP8Avg3(K, L, M);
      out[6] = out[8] = GWPVP8Avg2(L, M);
      out[7] = out[9] = GWPVP8Avg3(L, M, M);
      out[10] = out[11] = out[12] = out[13] = out[14] = out[15] = M;
      break;
  }
}

void GWPVP8AddResidual4x4(const GWPu8* pred,
                          GWPu32 pred_stride,
                          const int residue[16],
                          GWPu8* dst,
                          GWPu32 dst_stride) {
  GWPu32 r;
  GWPu32 c;
  if (pred == 0 || residue == 0 || dst == 0) return;
  for (r = 0u; r < 4u; ++r) {
    for (c = 0u; c < 4u; ++c) {
      int v;
      v = (int)pred[r * pred_stride + c] + residue[r * 4u + c];
      dst[r * dst_stride + c] = GWPClamp8(v);
    }
  }
}
