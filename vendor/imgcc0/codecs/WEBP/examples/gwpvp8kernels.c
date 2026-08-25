#include <stdio.h>

#include "../src/dec/vp8_filter.h"
#include "../src/dec/vp8_recon.h"
#include "../src/dec/vp8_transform.h"

static void PrintBlock(const char* key, const int* block, unsigned n) {
  unsigned i;
  int sum;
  sum = 0;
  for (i = 0u; i < n; ++i) sum += block[i];
  printf("%s.sum: %d\n", key, sum);
  printf("%s.first: %d\n", key, block[0]);
  printf("%s.last: %d\n", key, block[n - 1u]);
}

int main(void) {
  int dct_in[16];
  int dct_out[16];
  int wht_in[16];
  int wht_out[16];
  unsigned char pred4[16];
  unsigned char recon4[16];
  unsigned char above16[16];
  unsigned char left16[16];
  unsigned char pred16[16 * 16];
  unsigned char edge[8 * 8];
  int i;
  int interior_limit;
  int hev_threshold;

  for (i = 0; i < 16; ++i) {
    dct_in[i] = 0;
    wht_in[i] = 0;
    pred4[i] = 100u;
  }
  for (i = 0; i < 16; ++i) {
    above16[i] = (unsigned char)(60 + i);
    left16[i] = (unsigned char)(80 + i);
  }
  dct_in[0] = 32;
  wht_in[0] = 8;

  GWPVP8InverseDCT4x4(dct_in, dct_out);
  GWPVP8InverseWalsh4x4(wht_in, wht_out);
  GWPVP8AddResidual4x4(pred4, 4u, dct_out, recon4, 4u);
  GWPVP8Predict16x16(GWP_VP8_YMODE_TM, above16, left16, 55u, pred16);

  for (i = 0; i < 64; ++i) edge[i] = 100u;
  for (i = 0; i < 8; ++i) {
    edge[i * 8 + 2] = 90u;
    edge[i * 8 + 3] = 96u;
    edge[i * 8 + 4] = 128u;
    edge[i * 8 + 5] = 140u;
  }
  interior_limit = GWPVP8LoopFilterInteriorLimit(20, 0);
  hev_threshold = GWPVP8LoopFilterHevThreshold(20, 1);
  GWPVP8NormalMBFilterVerticalEdge(edge + 4, 8u, 20, interior_limit, hev_threshold, 8);

  PrintBlock("idct", dct_out, 16u);
  PrintBlock("wht", wht_out, 16u);
  printf("recon4.sum: %u\n", (unsigned)(
      recon4[0] + recon4[1] + recon4[2] + recon4[3] +
      recon4[4] + recon4[5] + recon4[6] + recon4[7] +
      recon4[8] + recon4[9] + recon4[10] + recon4[11] +
      recon4[12] + recon4[13] + recon4[14] + recon4[15]));
  printf("pred16.top_left: %u\n", (unsigned)pred16[0]);
  printf("pred16.center: %u\n", (unsigned)pred16[8 * 16 + 8]);
  printf("filter.edge_pixels: %u,%u,%u,%u\n",
         (unsigned)edge[3],
         (unsigned)edge[4],
         (unsigned)edge[11],
         (unsigned)edge[12]);
  return 0;
}
