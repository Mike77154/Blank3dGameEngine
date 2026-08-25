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
  char tuple[64];
  size_t pixels_size;
  unsigned char* pixels;
  f = fopen(path, "rb");
  if (f == 0) return 0;
  if (fgets(line, (int)sizeof(line), f) == 0 || strcmp(line, "P7\n") != 0) {
    fclose(f);
    return 0;
  }
  width = height = depth = maxval = 0u;
  tuple[0] = '\0';
  for (;;) {
    if (fgets(line, (int)sizeof(line), f) == 0) { fclose(f); return 0; }
    if (strcmp(line, "ENDHDR\n") == 0) break;
    if (sscanf(line, "WIDTH %u", &width) == 1) continue;
    if (sscanf(line, "HEIGHT %u", &height) == 1) continue;
    if (sscanf(line, "DEPTH %u", &depth) == 1) continue;
    if (sscanf(line, "MAXVAL %u", &maxval) == 1) continue;
    if (sscanf(line, "TUPLTYPE %63s", tuple) == 1) continue;
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
  unsigned int out_cap;
  unsigned char* out_data;
  unsigned char* scratch;
  unsigned int scratch_size;
  GWPEncodeConfig cfg;
  GWPStatusCode st;
  unsigned int out_size;
  const char* in_path;
  const char* out_path;
  int i;

  if (argc < 3) {
    fprintf(stderr, "uso: %s [--lossy] [--quality N] [--method N] [--preset NAME] [--alpha-q N] [--filter-strength N] [--partitions N] [--segments N] [--sharp-yuv] [--near-lossless N] [--no-skip] [--no-prob-updates] [--no-bmodes] [--no-uv-modes] [--no-intra16] [--dc-only] [--legacy-native] [--allow-tools] [--cwebp PATH] in.pam out.webp\n", argv[0]);
    return 1;
  }
  GWPEncodeConfigInit(&cfg);
  in_path = 0;
  out_path = 0;
  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--lossy") == 0) {
      cfg.lossless = GWP_FALSE;
    } else if (strcmp(argv[i], "--quality") == 0 && i + 1 < argc) {
      cfg.quality = (unsigned int)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--method") == 0 && i + 1 < argc) {
      cfg.method = (unsigned int)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--preset") == 0 && i + 1 < argc) {
      cfg.preset = ParsePreset(argv[++i]);
    } else if (strcmp(argv[i], "--alpha-q") == 0 && i + 1 < argc) {
      cfg.alpha_quality = (unsigned int)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--filter-strength") == 0 && i + 1 < argc) {
      cfg.filter_strength = (unsigned int)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--partitions") == 0 && i + 1 < argc) {
      cfg.vp8_partitions = (unsigned int)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--segments") == 0 && i + 1 < argc) {
      cfg.vp8_segments = (unsigned int)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--sharp-yuv") == 0) {
      cfg.use_sharp_yuv = GWP_TRUE;
    } else if (strcmp(argv[i], "--near-lossless") == 0 && i + 1 < argc) {
      cfg.near_lossless = (unsigned int)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--no-skip") == 0) {
      cfg.vp8_enable_coeff_skip = GWP_FALSE;
    } else if (strcmp(argv[i], "--no-prob-updates") == 0) {
      cfg.vp8_enable_prob_updates = GWP_FALSE;
    } else if (strcmp(argv[i], "--no-bmodes") == 0) {
      cfg.vp8_enable_subblock_modes = GWP_FALSE;
    } else if (strcmp(argv[i], "--no-uv-modes") == 0) {
      cfg.vp8_enable_uv_modes = GWP_FALSE;
    } else if (strcmp(argv[i], "--no-intra16") == 0) {
      cfg.vp8_enable_intra16_modes = GWP_FALSE;
    } else if (strcmp(argv[i], "--dc-only") == 0) {
      cfg.vp8_enable_ac_coeffs = GWP_FALSE;
    } else if (strcmp(argv[i], "--legacy-native") == 0) {
      cfg.vp8_partitions = 1u;
      cfg.vp8_segments = 1u;
      cfg.vp8_enable_coeff_skip = GWP_FALSE;
      cfg.vp8_enable_prob_updates = GWP_FALSE;
      cfg.vp8_enable_subblock_modes = GWP_FALSE;
      cfg.vp8_enable_uv_modes = GWP_FALSE;
      cfg.vp8_enable_intra16_modes = GWP_FALSE;
      cfg.vp8_enable_ac_coeffs = GWP_FALSE;
    } else if (strcmp(argv[i], "--allow-tools") == 0) {
      cfg.allow_external_tools = GWP_TRUE;
    } else if (strcmp(argv[i], "--cwebp") == 0 && i + 1 < argc) {
      cfg.cwebp_path = argv[++i];
      cfg.allow_external_tools = GWP_TRUE;
    } else if (in_path == 0) {
      in_path = argv[i];
    } else if (out_path == 0) {
      out_path = argv[i];
    } else {
      fprintf(stderr, "argumento no reconocido: %s\n", argv[i]);
      return 1;
    }
  }
  if (in_path == 0 || out_path == 0) {
    fprintf(stderr, "faltan in.pam / out.webp\n");
    return 1;
  }
  pixels = 0;
  if (!ReadPAMRGBA(in_path, &pixels, &w, &h)) {
    fprintf(stderr, "no pude leer PAM RGBA %s\n", in_path);
    return 1;
  }
  out_cap = cfg.lossless ? GWPEstimateWebPSizeFromPixels(w, h)
                         : GWPEstimateLossyWebPSizeFromPixels(w, h);
  scratch_size = GWPEstimateWebPSizeFromPixels(w, h) + GWPEstimateLossyWebPSizeFromPixels(w, h);
  if (GWPEstimateNativeLossyScratch(w, h) > scratch_size) {
    scratch_size = GWPEstimateNativeLossyScratch(w, h);
  }
  out_data = (unsigned char*)malloc(out_cap);
  scratch = (unsigned char*)malloc(scratch_size);
  if (out_data == 0 || scratch == 0) {
    fprintf(stderr, "memoria insuficiente\n");
    free(out_data);
    free(scratch);
    free(pixels);
    return 1;
  }
  cfg.output_buffer = out_data;
  cfg.output_buffer_size = out_cap;
  cfg.scratch = scratch;
  cfg.scratch_size = scratch_size;
  out_size = 0u;
  st = GWPEncodePixels(pixels, w, h, w * 4u, GWP_RAW_RGBA, &cfg, &out_size);
  if (st != GWP_STATUS_OK) {
    fprintf(stderr, "encode error: %d\n", (int)st);
    free(out_data);
    free(scratch);
    free(pixels);
    return 1;
  }
  if (!WriteFile(out_path, out_data, out_size)) {
    fprintf(stderr, "no pude escribir %s\n", out_path);
    free(out_data);
    free(scratch);
    free(pixels);
    return 1;
  }
  free(out_data);
  free(scratch);
  free(pixels);
  return 0;
}
