#include "vp8_transform.h"

#include "../utils/common.h"

#define GWP_VP8_COSPI8SQRT2MINUS1 20091
#define GWP_VP8_SINPI8SQRT2       35468

void GWPVP8DequantizeBlock(const int coeffs[16],
                           int dc_q,
                           int ac_q,
                           int out[16]) {
  int i;
  if (coeffs == 0 || out == 0) return;
  out[0] = coeffs[0] * dc_q;
  for (i = 1; i < 16; ++i) out[i] = coeffs[i] * ac_q;
}

void GWPVP8InverseWalsh4x4(const int input[16], int output[16]) {
  int tmp[16];
  int i;
  const int* ip;
  int* op;
  if (input == 0 || output == 0) return;
  ip = input;
  op = tmp;
  for (i = 0; i < 4; ++i) {
    int a1 = ip[0] + ip[12];
    int b1 = ip[4] + ip[8];
    int c1 = ip[4] - ip[8];
    int d1 = ip[0] - ip[12];
    op[0] = a1 + b1;
    op[4] = c1 + d1;
    op[8] = a1 - b1;
    op[12] = d1 - c1;
    ++ip;
    ++op;
  }
  ip = tmp;
  op = output;
  for (i = 0; i < 4; ++i) {
    int a1 = ip[0] + ip[3];
    int b1 = ip[1] + ip[2];
    int c1 = ip[1] - ip[2];
    int d1 = ip[0] - ip[3];
    int a2 = a1 + b1;
    int b2 = c1 + d1;
    int c2 = a1 - b1;
    int d2 = d1 - c1;
    op[0] = (a2 + 3) >> 3;
    op[1] = (b2 + 3) >> 3;
    op[2] = (c2 + 3) >> 3;
    op[3] = (d2 + 3) >> 3;
    ip += 4;
    op += 4;
  }
}

void GWPVP8InverseWalsh4x4DC(int dc, int output[16]) {
  int i;
  int a1;
  if (output == 0) return;
  a1 = (dc + 3) >> 3;
  for (i = 0; i < 16; ++i) output[i] = a1;
}

void GWPVP8InverseDCT4x4(const int input[16], int output[16]) {
  int tmp[16];
  int i;
  const int* ip;
  int* op;
  if (input == 0 || output == 0) return;
  ip = input;
  op = tmp;
  for (i = 0; i < 4; ++i) {
    int a1 = ip[0] + ip[8];
    int b1 = ip[0] - ip[8];
    int temp1 = (ip[4] * GWP_VP8_SINPI8SQRT2) >> 16;
    int temp2 = ip[12] + ((ip[12] * GWP_VP8_COSPI8SQRT2MINUS1) >> 16);
    int c1 = temp1 - temp2;
    temp1 = ip[4] + ((ip[4] * GWP_VP8_COSPI8SQRT2MINUS1) >> 16);
    temp2 = (ip[12] * GWP_VP8_SINPI8SQRT2) >> 16;
    op[0] = a1 + temp1 + temp2;
    op[12] = a1 - temp1 - temp2;
    op[4] = b1 + c1;
    op[8] = b1 - c1;
    ++ip;
    ++op;
  }
  ip = tmp;
  op = output;
  for (i = 0; i < 4; ++i) {
    int a1 = ip[0] + ip[2];
    int b1 = ip[0] - ip[2];
    int temp1 = (ip[1] * GWP_VP8_SINPI8SQRT2) >> 16;
    int temp2 = ip[3] + ((ip[3] * GWP_VP8_COSPI8SQRT2MINUS1) >> 16);
    int c1 = temp1 - temp2;
    temp1 = ip[1] + ((ip[1] * GWP_VP8_COSPI8SQRT2MINUS1) >> 16);
    temp2 = (ip[3] * GWP_VP8_SINPI8SQRT2) >> 16;
    op[0] = (a1 + temp1 + temp2 + 4) >> 3;
    op[3] = (a1 - temp1 - temp2 + 4) >> 3;
    op[1] = (b1 + c1 + 4) >> 3;
    op[2] = (b1 - c1 + 4) >> 3;
    ip += 4;
    op += 4;
  }
}

void GWPVP8InverseDCT4x4DC(int dc, int output[16]) {
  int i;
  int value;
  if (output == 0) return;
  value = (dc + 4) >> 3;
  for (i = 0; i < 16; ++i) output[i] = value;
}
