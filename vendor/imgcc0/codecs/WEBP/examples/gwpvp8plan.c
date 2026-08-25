#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/webp/encode.h"

static int ReadPAMRGBA(const char* path,
                       unsigned char** out_pixels,
                       unsigned int* out_w,
                       unsigned int* out_h) {
  FILE* f;
  char line[256];
  unsigned int width, height, depth, maxval;
  size_t pixels_size;
  unsigned char* pixels;
  f = fopen(path, "rb");
  if (f == 0) return 0;
  if (fgets(line, (int)sizeof(line), f) == 0 || strcmp(line, "P7\n") != 0) {
    fclose(f);
    return 0;
  }
  width = height = depth = maxval = 0u;
  for (;;) {
    if (fgets(line, (int)sizeof(line), f) == 0) { fclose(f); return 0; }
    if (strcmp(line, "ENDHDR\n") == 0) break;
    if (sscanf(line, "WIDTH %u", &width) == 1) continue;
    if (sscanf(line, "HEIGHT %u", &height) == 1) continue;
    if (sscanf(line, "DEPTH %u", &depth) == 1) continue;
    if (sscanf(line, "MAXVAL %u", &maxval) == 1) continue;
  }
  if (width == 0u || height == 0u || depth != 4u || maxval != 255u) {
    fclose(f);
    return 0;
  }
  pixels_size = (size_t)width * (size_t)height * 4u;
  pixels = (unsigned char*)malloc(pixels_size);
  if (pixels == 0) { fclose(f); return 0; }
  if (fread(pixels, 1, pixels_size, f) != pixels_size) {
    free(pixels);
    fclose(f);
    return 0;
  }
  fclose(f);
  *out_pixels = pixels;
  *out_w = width;
  *out_h = height;
  return 1;
}

static GWPEncodePreset ParsePreset(const char* text) {
  if (text == 0) return GWP_PRESET_DEFAULT;
  if (strcmp(text, "photo") == 0) return GWP_PRESET_PHOTO;
  if (strcmp(text, "picture") == 0) return GWP_PRESET_PICTURE;
  if (strcmp(text, "drawing") == 0) return GWP_PRESET_DRAWING;
  if (strcmp(text, "icon") == 0) return GWP_PRESET_ICON;
  if (strcmp(text, "text") == 0) return GWP_PRESET_TEXT;
  return GWP_PRESET_DEFAULT;
}

int main(int argc, char** argv) {
  unsigned char* pixels;
  unsigned int w, h;
  unsigned int scratch_size;
  unsigned char* scratch;
  GWPEncodeConfig cfg;
  GWPVP8LossyPlan plan;
  const char* in_path;
  int dump_mbs;
  int i;

  if (argc < 2) {
    fprintf(stderr, "uso: %s [--quality N] [--preset NAME] [--dump-mbs] in.pam\n", argv[0]);
    return 1;
  }

  GWPEncodeConfigInit(&cfg);
  cfg.lossless = GWP_FALSE;
  in_path = 0;
  dump_mbs = 0;
  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--quality") == 0 && i + 1 < argc) {
      cfg.quality = (unsigned int)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--preset") == 0 && i + 1 < argc) {
      cfg.preset = ParsePreset(argv[++i]);
    } else if (strcmp(argv[i], "--dump-mbs") == 0) {
      dump_mbs = 1;
    } else if (in_path == 0) {
      in_path = argv[i];
    } else {
      fprintf(stderr, "argumento no reconocido: %s\n", argv[i]);
      return 1;
    }
  }
  if (in_path == 0) {
    fprintf(stderr, "falta in.pam\n");
    return 1;
  }

  pixels = 0;
  if (!ReadPAMRGBA(in_path, &pixels, &w, &h)) {
    fprintf(stderr, "no pude leer PAM RGBA %s\n", in_path);
    return 1;
  }
  scratch_size = GWPEstimateVP8LossyPlanScratch(w, h);
  scratch = (unsigned char*)malloc(scratch_size);
  if (scratch == 0) {
    fprintf(stderr, "memoria insuficiente\n");
    free(pixels);
    return 1;
  }
  if (GWPAnalyzeVP8LossyPlan(pixels, w, h, w * 4u, GWP_RAW_RGBA,
                             &cfg, scratch, scratch_size, &plan) != GWP_STATUS_OK) {
    fprintf(stderr, "no pude analizar frame\n");
    free(scratch);
    free(pixels);
    return 1;
  }

  printf("width=%u\n", plan.width);
  printf("height=%u\n", plan.height);
  printf("mb_cols=%u\n", plan.mb_cols);
  printf("mb_rows=%u\n", plan.mb_rows);
  printf("macroblocks=%u\n", plan.macroblock_count);
  printf("avg_luma=%u\n", plan.average_luma);
  printf("avg_variance=%u\n", plan.average_variance);
  printf("alpha_macroblocks=%u\n", plan.alpha_macroblocks);
  printf("suggested_segments=%u\n", plan.suggested_segments);
  printf("suggested_partitions=%u\n", plan.suggested_partitions);
  printf("suggested_filter_strength=%u\n", plan.suggested_filter_strength);
  printf("suggested_sharpness=%u\n", plan.suggested_sharpness);

  if (dump_mbs) {
    unsigned int mb;
    for (mb = 0u; mb < plan.macroblock_count; ++mb) {
      const GWPVP8LossyMacroblockStat* st;
      st = &plan.macroblocks[mb];
      printf("mb[%u] mean=%u var=%u seg=%u filter=%u alpha=%u\n",
             mb,
             (unsigned int)st->mean_y,
             (unsigned int)st->variance_y,
             (unsigned int)st->segment_id,
             (unsigned int)st->filter_strength,
             (unsigned int)st->has_alpha);
    }
  }

  free(scratch);
  free(pixels);
  return 0;
}
