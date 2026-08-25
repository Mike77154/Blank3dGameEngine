#include <stdio.h>
#include <stdlib.h>

#include "../src/webp/demux.h"
#include "../src/dec/vp8_dec.h"

static long ReadFile(const char* path, unsigned char** out_data) {
  FILE* f;
  long size;
  unsigned char* data;
  f = fopen(path, "rb");
  if (f == 0) return -1;
  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
  size = ftell(f);
  if (size < 0) { fclose(f); return -1; }
  if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return -1; }
  data = (unsigned char*)malloc((size_t)size);
  if (data == 0) { fclose(f); return -1; }
  if (fread(data, 1, (size_t)size, f) != (size_t)size) {
    free(data);
    fclose(f);
    return -1;
  }
  fclose(f);
  *out_data = data;
  return size;
}

static void PrintSegments(const GWPVP8ControlHeader* vp8) {
  unsigned i;
  printf("segment.enabled: %d\n", (int)vp8->segment.enabled);
  printf("segment.update_map: %d\n", (int)vp8->segment.update_map);
  printf("segment.update_data: %d\n", (int)vp8->segment.update_data);
  printf("segment.absolute_delta: %d\n", (int)vp8->segment.absolute_delta);
  for (i = 0u; i < GWP_VP8_MAX_SEGMENTS; ++i) {
    printf("segment[%u].quant_idx: %d\n", i, vp8->segment.quant_idx[i]);
    printf("segment[%u].lf_level: %d\n", i, vp8->segment.lf_level[i]);
  }
  for (i = 0u; i < GWP_VP8_MB_FEATURE_TREE_PROBS; ++i) {
    printf("segment.tree_prob[%u]: %u\n", i, (unsigned)vp8->segment.tree_probs[i]);
  }
}

static void PrintLoopFilter(const GWPVP8ControlHeader* vp8) {
  unsigned i;
  printf("loop_filter.use_simple: %d\n", (int)vp8->loop_filter.use_simple);
  printf("loop_filter.level: %u\n", (unsigned)vp8->loop_filter.level);
  printf("loop_filter.sharpness: %u\n", (unsigned)vp8->loop_filter.sharpness);
  printf("loop_filter.delta_enabled: %d\n", (int)vp8->loop_filter.delta_enabled);
  for (i = 0u; i < GWP_VP8_BLOCK_CONTEXTS; ++i) {
    printf("loop_filter.ref_delta[%u]: %d\n", i, vp8->loop_filter.ref_delta[i]);
  }
  for (i = 0u; i < GWP_VP8_BLOCK_CONTEXTS; ++i) {
    printf("loop_filter.mode_delta[%u]: %d\n", i, vp8->loop_filter.mode_delta[i]);
  }
}

static void PrintTokenPartitions(const GWPVP8ControlHeader* vp8) {
  unsigned i;
  printf("token_partitions.count: %u\n", (unsigned)vp8->token_partitions.partition_count);
  for (i = 0u; i < vp8->token_partitions.partition_count; ++i) {
    printf("token_partition[%u].size: %u\n",
           i,
           (unsigned)vp8->token_partitions.partition[i].size);
  }
}

static void PrintQuant(const GWPVP8ControlHeader* vp8) {
  printf("quant.q_index: %u\n", (unsigned)vp8->quant.q_index);
  printf("quant.y1_dc_delta_q: %d\n", vp8->quant.y1_dc_delta_q);
  printf("quant.y2_dc_delta_q: %d\n", vp8->quant.y2_dc_delta_q);
  printf("quant.y2_ac_delta_q: %d\n", vp8->quant.y2_ac_delta_q);
  printf("quant.uv_dc_delta_q: %d\n", vp8->quant.uv_dc_delta_q);
  printf("quant.uv_ac_delta_q: %d\n", vp8->quant.uv_ac_delta_q);
}

static void PrintReference(const GWPVP8ControlHeader* vp8) {
  printf("reference.refresh_gf: %d\n", (int)vp8->reference.refresh_gf);
  printf("reference.refresh_arf: %d\n", (int)vp8->reference.refresh_arf);
  printf("reference.copy_gf: %u\n", (unsigned)vp8->reference.copy_gf);
  printf("reference.copy_arf: %u\n", (unsigned)vp8->reference.copy_arf);
  printf("reference.sign_bias_golden: %u\n", (unsigned)vp8->reference.sign_bias_golden);
  printf("reference.sign_bias_altref: %u\n", (unsigned)vp8->reference.sign_bias_altref);
  printf("reference.refresh_entropy: %d\n", (int)vp8->reference.refresh_entropy);
  printf("reference.refresh_last: %d\n", (int)vp8->reference.refresh_last);
}

static void PrintDequant(const GWPVP8ControlHeader* vp8) {
  unsigned i;
  for (i = 0u; i < GWP_VP8_MAX_SEGMENTS; ++i) {
    printf("dequant[%u].quant_idx: %d\n", i, vp8->dequant[i].quant_idx);
    printf("dequant[%u].y1_dc: %d\n", i, vp8->dequant[i].y1_dc);
    printf("dequant[%u].y1_ac: %d\n", i, vp8->dequant[i].y1_ac);
    printf("dequant[%u].y2_dc: %d\n", i, vp8->dequant[i].y2_dc);
    printf("dequant[%u].y2_ac: %d\n", i, vp8->dequant[i].y2_ac);
    printf("dequant[%u].uv_dc: %d\n", i, vp8->dequant[i].uv_dc);
    printf("dequant[%u].uv_ac: %d\n", i, vp8->dequant[i].uv_ac);
  }
}

static void PrintEntropy(const GWPVP8ControlHeader* vp8) {
  printf("entropy.coeff_update_count: %u\n", (unsigned)vp8->entropy.coeff_update_count);
  printf("entropy.coeff_skip_enabled: %d\n", (int)vp8->entropy.coeff_skip_enabled);
  printf("entropy.coeff_skip_prob: %u\n", (unsigned)vp8->entropy.coeff_skip_prob);
  printf("entropy.y_mode_probs: %u,%u,%u,%u\n",
         (unsigned)vp8->entropy.y_mode_probs[0],
         (unsigned)vp8->entropy.y_mode_probs[1],
         (unsigned)vp8->entropy.y_mode_probs[2],
         (unsigned)vp8->entropy.y_mode_probs[3]);
  printf("entropy.uv_mode_probs: %u,%u,%u\n",
         (unsigned)vp8->entropy.uv_mode_probs[0],
         (unsigned)vp8->entropy.uv_mode_probs[1],
         (unsigned)vp8->entropy.uv_mode_probs[2]);
}


static void PrintResidualSummary(const GWPVP8ControlHeader* vp8) {
  unsigned i;
  printf("residual.macroblock_count: %u\n", (unsigned)vp8->residual_summary.macroblock_count);
  printf("residual.skipped_macroblocks: %u\n", (unsigned)vp8->residual_summary.skipped_macroblocks);
  printf("residual.y2_macroblocks: %u\n", (unsigned)vp8->residual_summary.y2_macroblocks);
  printf("residual.token_partition_count: %u\n", (unsigned)vp8->residual_summary.token_partition_count);
  printf("residual.coeff_block_count: %u\n", (unsigned)vp8->residual_summary.coeff_block_count);
  printf("residual.blocks_with_coeffs: %u\n", (unsigned)vp8->residual_summary.blocks_with_coeffs);
  printf("residual.y_blocks_with_coeffs: %u\n", (unsigned)vp8->residual_summary.y_blocks_with_coeffs);
  printf("residual.uv_blocks_with_coeffs: %u\n", (unsigned)vp8->residual_summary.uv_blocks_with_coeffs);
  printf("residual.y2_blocks_with_coeffs: %u\n", (unsigned)vp8->residual_summary.y2_blocks_with_coeffs);
  printf("residual.nonzero_coeff_count: %u\n", (unsigned)vp8->residual_summary.nonzero_coeff_count);
  printf("residual.max_abs_coeff: %u\n", (unsigned)vp8->residual_summary.max_abs_coeff);
  for (i = 0u; i < GWP_VP8_TOKEN_COUNT; ++i) {
    printf("residual.token_hist[%u]: %u\n",
           i,
           (unsigned)vp8->residual_summary.token_hist[i]);
  }
  for (i = 0u; i < vp8->residual_summary.token_partition_count; ++i) {
    printf("residual.token_partition_bytes_touched[%u]: %u\n",
           i,
           (unsigned)vp8->residual_summary.token_partition_bytes_touched[i]);
  }
}

static void PrintModeSummary(const GWPVP8ControlHeader* vp8) {
  unsigned i;
  printf("modes.macroblock_count: %u\n", (unsigned)vp8->mode_summary.macroblock_count);
  printf("modes.skipped_macroblocks: %u\n", (unsigned)vp8->mode_summary.skipped_macroblocks);
  printf("modes.bpred_macroblocks: %u\n", (unsigned)vp8->mode_summary.bpred_macroblocks);
  for (i = 0u; i < GWP_VP8_MAX_SEGMENTS; ++i) {
    printf("modes.segment_hist[%u]: %u\n", i, (unsigned)vp8->mode_summary.segment_hist[i]);
  }
  for (i = 0u; i < GWP_VP8_Y_MODE_COUNT; ++i) {
    printf("modes.y_mode_hist[%u]: %u\n", i, (unsigned)vp8->mode_summary.y_mode_hist[i]);
  }
  for (i = 0u; i < GWP_VP8_UV_MODE_COUNT; ++i) {
    printf("modes.uv_mode_hist[%u]: %u\n", i, (unsigned)vp8->mode_summary.uv_mode_hist[i]);
  }
  for (i = 0u; i < GWP_VP8_B_MODE_COUNT; ++i) {
    printf("modes.b_mode_hist[%u]: %u\n", i, (unsigned)vp8->mode_summary.b_mode_hist[i]);
  }
}

int main(int argc, char** argv) {
  unsigned char* data;
  long file_size;
  GWPDemuxer dmux;
  GWPVP8ControlHeader vp8;
  GWPStatusCode st;

  if (argc != 2) {
    fprintf(stderr, "uso: %s image.webp\n", argv[0]);
    return 1;
  }

  data = 0;
  file_size = ReadFile(argv[1], &data);
  if (file_size < 0) {
    fprintf(stderr, "no pude leer %s\n", argv[1]);
    return 1;
  }

  st = GWPDemuxParse(&dmux, data, (GWPu32)file_size);
  if (st != GWP_STATUS_OK) {
    fprintf(stderr, "demux error: %d\n", (int)st);
    free(data);
    return 1;
  }
  if (dmux.features.format != GWP_BITSTREAM_VP8 || dmux.vp8_payload.bytes == 0) {
    fprintf(stderr, "no es un WebP VP8 lossy\n");
    free(data);
    return 1;
  }

  st = GWPVP8ParseControlHeader(dmux.vp8_payload.bytes, dmux.vp8_payload.size, &vp8);
  if (st != GWP_STATUS_OK) {
    fprintf(stderr, "vp8 control header error: %d\n", (int)st);
    free(data);
    return 1;
  }

  printf("key_frame: %d\n", (int)vp8.frame.tag.key_frame);
  printf("version: %u\n", (unsigned)vp8.frame.tag.version);
  printf("show_frame: %d\n", (int)vp8.frame.tag.show_frame);
  printf("first_partition_size: %u\n", (unsigned)vp8.frame.tag.first_partition_size);
  printf("width: %u\n", (unsigned)vp8.frame.key.width);
  printf("height: %u\n", (unsigned)vp8.frame.key.height);
  printf("horizontal_scale: %u\n", (unsigned)vp8.frame.key.horizontal_scale);
  printf("vertical_scale: %u\n", (unsigned)vp8.frame.key.vertical_scale);
  printf("mb_cols: %u\n", (unsigned)vp8.mb_cols);
  printf("mb_rows: %u\n", (unsigned)vp8.mb_rows);
  printf("color_space: %u\n", (unsigned)vp8.color_space);
  printf("clamping_type: %u\n", (unsigned)vp8.clamping_type);
  printf("control_header_bytes_touched: %u\n", (unsigned)vp8.control_header_bytes_touched);
  printf("entropy_header_bytes_touched: %u\n", (unsigned)vp8.entropy_header_bytes_touched);
  printf("part0_bytes_touched: %u\n", (unsigned)vp8.part0_bytes_touched);

  PrintSegments(&vp8);
  PrintLoopFilter(&vp8);
  PrintTokenPartitions(&vp8);
  PrintQuant(&vp8);
  PrintReference(&vp8);
  PrintEntropy(&vp8);
  PrintModeSummary(&vp8);
  PrintResidualSummary(&vp8);
  PrintDequant(&vp8);

  free(data);
  return 0;
}
