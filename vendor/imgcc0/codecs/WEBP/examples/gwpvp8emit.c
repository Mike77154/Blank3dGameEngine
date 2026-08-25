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

static int WriteFile(const char* path, const unsigned char* data, unsigned int size) {
  FILE* f;
  f = fopen(path, "wb");
  if (f == 0) return 0;
  if (fwrite(data, 1, size, f) != size) {
    fclose(f);
    return 0;
  }
  fclose(f);
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
  unsigned char* scratch;
  unsigned int scratch_size;
  GWPEncodeConfig cfg;
  GWPVP8IntraEmitPlan plan;
  const char* in_path;
  const char* proxy_webp;
  int dump_json;
  int i;

  if (argc < 2) {
    fprintf(stderr, "uso: %s [--quality N] [--preset NAME] [--dump-json] [--proxy-webp out.webp] in.pam\n", argv[0]);
    return 1;
  }
  GWPEncodeConfigInit(&cfg);
  cfg.lossless = GWP_FALSE;
  in_path = 0;
  proxy_webp = 0;
  dump_json = 0;
  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--quality") == 0 && i + 1 < argc) {
      cfg.quality = (unsigned int)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--preset") == 0 && i + 1 < argc) {
      cfg.preset = ParsePreset(argv[++i]);
    } else if (strcmp(argv[i], "--dump-json") == 0) {
      dump_json = 1;
    } else if (strcmp(argv[i], "--proxy-webp") == 0 && i + 1 < argc) {
      proxy_webp = argv[++i];
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
    fprintf(stderr, "no pude leer %s\n", in_path);
    return 1;
  }
  scratch_size = GWPEstimateNativeLossyScratch(w, h);
  scratch = (unsigned char*)malloc(scratch_size);
  if (scratch == 0) {
    fprintf(stderr, "memoria insuficiente\n");
    free(pixels);
    return 1;
  }
  if (GWPBuildVP8IntraEmitPlan(pixels, w, h, w * 4u, GWP_RAW_RGBA,
                               &cfg, scratch, scratch_size, &plan) != GWP_STATUS_OK) {
    fprintf(stderr, "no pude construir emisor intra-only\n");
    free(scratch);
    free(pixels);
    return 1;
  }

  if (dump_json) {
    unsigned int j;
    printf("{\n");
    printf("  \"width\": %u,\n", plan.width);
    printf("  \"height\": %u,\n", plan.height);
    printf("  \"mb_cols\": %u,\n", plan.mb_cols);
    printf("  \"mb_rows\": %u,\n", plan.mb_rows);
    printf("  \"cell_count\": %u,\n", plan.cell_count);
    printf("  \"average_qindex\": %u,\n", plan.average_qindex);
    printf("  \"suggested_partitions\": %u,\n", plan.suggested_partitions);
    printf("  \"suggested_filter_strength\": %u,\n", plan.suggested_filter_strength);
    printf("  \"cells\": [\n");
    for (j = 0u; j < plan.cell_count; ++j) {
      const GWPVP8IntraCell* c;
      c = &plan.cells[j];
      printf("    {\"x\":%u,\"y\":%u,\"w\":%u,\"h\":%u,\"seg\":%u,\"mode\":%u,\"q\":%u,\"rgba\":[%u,%u,%u,%u]}%s\n",
             (unsigned int)c->x,
             (unsigned int)c->y,
             (unsigned int)c->width,
             (unsigned int)c->height,
             (unsigned int)c->segment_id,
             (unsigned int)c->mode,
             (unsigned int)c->qindex,
             (unsigned int)c->r,
             (unsigned int)c->g,
             (unsigned int)c->b,
             (unsigned int)c->a,
             (j + 1u == plan.cell_count) ? "" : ",");
    }
    printf("  ]\n");
    printf("}\n");
  } else {
    printf("width=%u\n", plan.width);
    printf("height=%u\n", plan.height);
    printf("cell_count=%u\n", plan.cell_count);
    printf("average_qindex=%u\n", plan.average_qindex);
    printf("suggested_partitions=%u\n", plan.suggested_partitions);
    printf("suggested_filter_strength=%u\n", plan.suggested_filter_strength);
  }

  if (proxy_webp != 0) {
    unsigned int out_cap;
    unsigned char* out_data;
    unsigned int out_size;
    out_cap = GWPEstimateLossyWebPSizeFromPixels(w, h);
    out_data = (unsigned char*)malloc(out_cap);
    if (out_data == 0) {
      fprintf(stderr, "memoria insuficiente para proxy\n");
      free(scratch);
      free(pixels);
      return 1;
    }
    cfg.output_buffer = out_data;
    cfg.output_buffer_size = out_cap;
    cfg.scratch = scratch;
    cfg.scratch_size = scratch_size;
    out_size = 0u;
    if (GWPEncodeLossyNativeProxy(pixels, w, h, w * 4u, GWP_RAW_RGBA, &cfg, &out_size) != GWP_STATUS_OK) {
      fprintf(stderr, "no pude emitir proxy native-lossy\n");
      free(out_data);
      free(scratch);
      free(pixels);
      return 1;
    }
    if (!WriteFile(proxy_webp, out_data, out_size)) {
      fprintf(stderr, "no pude escribir %s\n", proxy_webp);
      free(out_data);
      free(scratch);
      free(pixels);
      return 1;
    }
    free(out_data);
  }

  free(scratch);
  free(pixels);
  return 0;
}
