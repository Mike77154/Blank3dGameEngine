#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/webp/anim_encode.h"

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
  unsigned char** frames;
  unsigned int* widths;
  unsigned int* heights;
  unsigned int frame_count;
  unsigned int i;
  unsigned int w, h;
  GWPAnimEncoderOptions opt;
  GWPAnimEncoder enc;
  GWPAnimFrameSpec spec;
  GWPEncodeConfig cfg;
  GWPMuxAnimParams anim_params;
  unsigned char* work_mem;
  unsigned int work_size;
  unsigned char* encode_scratch;
  unsigned int encode_scratch_size;
  unsigned char* out_data;
  unsigned int out_cap;
  unsigned int out_size;
  const char* out_path;
  int argi;
  int frame_base;

  if (argc < 4) {
    fprintf(stderr, "uso: %s [--mixed] [--lossy] [--quality N] [--method N] [--preset NAME] [--alpha-q N] [--filter-strength N] [--partitions N] [--segments N] [--sharp-yuv] [--no-skip] [--no-prob-updates] [--no-bmodes] [--no-uv-modes] [--no-intra16] [--dc-only] [--legacy-native] [--kmin N] [--kmax N] [--loop N] [--min-size|--no-min-size] [--allow-tools] [--cwebp PATH] out.webp frame0.pam frame1.pam [...frameN.pam]\n", argv[0]);
    return 1;
  }
  GWPAnimEncoderOptionsInit(&opt);
  GWPEncodeConfigInit(&cfg);
  anim_params.bgcolor = 0u;
  anim_params.loop_count = 0u;
  out_path = 0;
  frame_base = 0;
  for (argi = 1; argi < argc; ++argi) {
    if (strcmp(argv[argi], "--mixed") == 0) {
      opt.allow_mixed = GWP_TRUE;
      cfg.lossless = GWP_FALSE;
    } else if (strcmp(argv[argi], "--lossy") == 0) {
      cfg.lossless = GWP_FALSE;
    } else if (strcmp(argv[argi], "--quality") == 0 && argi + 1 < argc) {
      cfg.quality = (unsigned int)atoi(argv[++argi]);
    } else if (strcmp(argv[argi], "--method") == 0 && argi + 1 < argc) {
      cfg.method = (unsigned int)atoi(argv[++argi]);
    } else if (strcmp(argv[argi], "--preset") == 0 && argi + 1 < argc) {
      cfg.preset = ParsePreset(argv[++argi]);
    } else if (strcmp(argv[argi], "--alpha-q") == 0 && argi + 1 < argc) {
      cfg.alpha_quality = (unsigned int)atoi(argv[++argi]);
    } else if (strcmp(argv[argi], "--filter-strength") == 0 && argi + 1 < argc) {
      cfg.filter_strength = (unsigned int)atoi(argv[++argi]);
    } else if (strcmp(argv[argi], "--partitions") == 0 && argi + 1 < argc) {
      cfg.vp8_partitions = (unsigned int)atoi(argv[++argi]);
    } else if (strcmp(argv[argi], "--segments") == 0 && argi + 1 < argc) {
      cfg.vp8_segments = (unsigned int)atoi(argv[++argi]);
    } else if (strcmp(argv[argi], "--sharp-yuv") == 0) {
      cfg.use_sharp_yuv = GWP_TRUE;
    } else if (strcmp(argv[argi], "--no-skip") == 0) {
      cfg.vp8_enable_coeff_skip = GWP_FALSE;
    } else if (strcmp(argv[argi], "--no-prob-updates") == 0) {
      cfg.vp8_enable_prob_updates = GWP_FALSE;
    } else if (strcmp(argv[argi], "--no-bmodes") == 0) {
      cfg.vp8_enable_subblock_modes = GWP_FALSE;
    } else if (strcmp(argv[argi], "--no-uv-modes") == 0) {
      cfg.vp8_enable_uv_modes = GWP_FALSE;
    } else if (strcmp(argv[argi], "--no-intra16") == 0) {
      cfg.vp8_enable_intra16_modes = GWP_FALSE;
    } else if (strcmp(argv[argi], "--dc-only") == 0) {
      cfg.vp8_enable_ac_coeffs = GWP_FALSE;
    } else if (strcmp(argv[argi], "--legacy-native") == 0) {
      cfg.vp8_partitions = 1u;
      cfg.vp8_segments = 1u;
      cfg.vp8_enable_coeff_skip = GWP_FALSE;
      cfg.vp8_enable_prob_updates = GWP_FALSE;
      cfg.vp8_enable_subblock_modes = GWP_FALSE;
      cfg.vp8_enable_uv_modes = GWP_FALSE;
      cfg.vp8_enable_intra16_modes = GWP_FALSE;
      cfg.vp8_enable_ac_coeffs = GWP_FALSE;
    } else if (strcmp(argv[argi], "--kmin") == 0 && argi + 1 < argc) {
      opt.kmin = (unsigned int)atoi(argv[++argi]);
    } else if (strcmp(argv[argi], "--kmax") == 0 && argi + 1 < argc) {
      opt.kmax = (unsigned int)atoi(argv[++argi]);
    } else if (strcmp(argv[argi], "--loop") == 0 && argi + 1 < argc) {
      anim_params.loop_count = (unsigned int)atoi(argv[++argi]);
    } else if (strcmp(argv[argi], "--min-size") == 0) {
      opt.minimize_size = GWP_TRUE;
    } else if (strcmp(argv[argi], "--no-min-size") == 0) {
      opt.minimize_size = GWP_FALSE;
    } else if (strcmp(argv[argi], "--allow-tools") == 0) {
      cfg.allow_external_tools = GWP_TRUE;
    } else if (strcmp(argv[argi], "--cwebp") == 0 && argi + 1 < argc) {
      cfg.cwebp_path = argv[++argi];
      cfg.allow_external_tools = GWP_TRUE;
    } else {
      out_path = argv[argi];
      frame_base = argi + 1;
      break;
    }
  }
  if (out_path == 0 || frame_base >= argc) {
    fprintf(stderr, "faltan output y frames\n");
    return 1;
  }

  frame_count = (unsigned int)(argc - frame_base);
  frames = (unsigned char**)calloc(frame_count, sizeof(*frames));
  widths = (unsigned int*)calloc(frame_count, sizeof(*widths));
  heights = (unsigned int*)calloc(frame_count, sizeof(*heights));
  if (frames == 0 || widths == 0 || heights == 0) {
    fprintf(stderr, "memoria insuficiente\n");
    free(frames); free(widths); free(heights);
    return 1;
  }

  w = h = 0u;
  for (i = 0u; i < frame_count; ++i) {
    if (!ReadPAMRGBA(argv[frame_base + i], &frames[i], &widths[i], &heights[i])) {
      fprintf(stderr, "no pude leer %s\n", argv[frame_base + i]);
      return 1;
    }
    if (i == 0u) {
      w = widths[i];
      h = heights[i];
    } else if (widths[i] != w || heights[i] != h) {
      fprintf(stderr, "todas las frames deben medir igual\n");
      return 1;
    }
  }

  if (opt.kmin == 0u && opt.kmax == 0u && !opt.minimize_size) {
    if (cfg.lossless) {
      opt.kmin = 9u;
      opt.kmax = 17u;
    } else {
      opt.kmin = 3u;
      opt.kmax = 5u;
    }
  }
  if (opt.minimize_size) {
    opt.kmin = 0u;
    opt.kmax = 0u;
  }

  work_size = w * h * 4u + frame_count * (GWPEstimateLosslessBitstreamSize(w, h) + GWPEstimateLossyWebPSizeFromPixels(w, h));
  encode_scratch_size = GWPEstimateNativeLossyScratch(w, h);
  if (encode_scratch_size < GWPEstimateWebPSizeFromPixels(w, h) + GWPEstimateLossyWebPSizeFromPixels(w, h)) {
    encode_scratch_size = GWPEstimateWebPSizeFromPixels(w, h) + GWPEstimateLossyWebPSizeFromPixels(w, h);
  }
  work_mem = (unsigned char*)malloc(work_size);
  encode_scratch = (unsigned char*)malloc(encode_scratch_size);
  out_cap = 1024u + frame_count * GWPEstimateWebPSizeFromPixels(w, h);
  out_data = (unsigned char*)malloc(out_cap);
  if (work_mem == 0 || encode_scratch == 0 || out_data == 0) {
    fprintf(stderr, "memoria insuficiente\n");
    return 1;
  }
  opt.work_mem = work_mem;
  opt.work_mem_size = work_size;
  GWPAnimEncoderInit(&enc, w, h, &opt);
  GWPAnimEncoderSetAnimationParams(&enc, &anim_params);
  GWPAnimFrameSpecInit(&spec);
  cfg.scratch = encode_scratch;
  cfg.scratch_size = encode_scratch_size;

  for (i = 0u; i < frame_count; ++i) {
    GWPMuxError mx;
    mx = GWPAnimEncoderAddFramePixels(&enc, frames[i], w * 4u, GWP_RAW_RGBA, &spec, &cfg);
    if (mx != GWP_MUX_OK) {
      fprintf(stderr, "anim encode error en frame %u: %d\n", i, (int)mx);
      return 1;
    }
  }

  if (GWPAnimEncoderAssemble(&enc, out_data, out_cap, &out_size) != GWP_MUX_OK) {
    fprintf(stderr, "no pude ensamblar animacion\n");
    return 1;
  }
  if (!WriteFile(out_path, out_data, out_size)) {
    fprintf(stderr, "no pude escribir %s\n", out_path);
    return 1;
  }

  for (i = 0u; i < frame_count; ++i) free(frames[i]);
  free(frames);
  free(widths);
  free(heights);
  free(work_mem);
  free(encode_scratch);
  free(out_data);
  return 0;
}
