#include "tool_bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
extern int mkstemp(char*);
#endif

#include "../utils/common.h"

static int GWPWriteFileBridge(const char* path, const GWPu8* data, GWPu32 size) {
  FILE* f;
  f = fopen(path, "wb");
  if (f == 0) return 0;
  if (size != 0u && fwrite(data, 1u, size, f) != size) {
    fclose(f);
    return 0;
  }
  fclose(f);
  return 1;
}

static int GWPReadFileBridge(const char* path, GWPu8* data, GWPu32 capacity, GWPu32* out_size) {
  FILE* f;
  long size;
  if (out_size != 0) *out_size = 0u;
  f = fopen(path, "rb");
  if (f == 0) return 0;
  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
  size = ftell(f);
  if (size < 0 || (GWPu32)size > capacity) { fclose(f); return 0; }
  if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
  if ((GWPu32)size != 0u && fread(data, 1u, (size_t)size, f) != (size_t)size) {
    fclose(f);
    return 0;
  }
  fclose(f);
  if (out_size != 0) *out_size = (GWPu32)size;
  return 1;
}

static const char* GWPAlphaFilterArg(GWPAlphaFilteringMode mode) {
  switch (mode) {
    case GWP_ALPHA_FILTER_NONE: return "none";
    case GWP_ALPHA_FILTER_BEST: return "best";
    default: return "fast";
  }
}

static const char* GWPPresetArg(GWPEncodePreset preset) {
  switch (preset) {
    case GWP_PRESET_PHOTO: return "photo";
    case GWP_PRESET_PICTURE: return "picture";
    case GWP_PRESET_DRAWING: return "drawing";
    case GWP_PRESET_ICON: return "icon";
    case GWP_PRESET_TEXT: return "text";
    default: return "default";
  }
}

static int GWPMakeTempBase(char* out_base) {
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
  int fd;
  strcpy(out_base, "/tmp/gwpXXXXXX");
  fd = mkstemp(out_base);
  if (fd < 0) return 0;
  close(fd);
  remove(out_base);
  return 1;
#else
  return (tmpnam(out_base) != 0);
#endif
}

GWPStatusCode GWPEncodeViaCWebP(const GWPu8* pixels,
                                GWPu32 width,
                                GWPu32 height,
                                GWPu32 stride,
                                GWPRawPixelFormat pixel_format,
                                const GWPEncodeConfig* config,
                                GWPu32* out_size) {
  char tmp_base[L_tmpnam + 32];
  char input_path[L_tmpnam + 48];
  char output_path[L_tmpnam + 48];
  char command[1400];
  char preset_arg[64];
  char filter_arg[64];
  char alpha_q_arg[64];
  GWPEncodeConfig temp_cfg;
  GWPu32 temp_size;
  const char* cwebp;
  int rc;
  if (out_size != 0) *out_size = 0u;
  if (pixels == 0 || config == 0 || out_size == 0) return GWP_STATUS_INVALID_PARAM;
  if (!config->allow_external_tools) return GWP_STATUS_NOT_IMPLEMENTED;
  if (config->scratch == 0 || config->scratch_size < GWPEstimateWebPSizeFromPixels(width, height)) {
    return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  }
  if (config->output_buffer == 0 || config->output_buffer_size == 0u) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (!GWPMakeTempBase(tmp_base)) return GWP_STATUS_NOT_IMPLEMENTED;
  sprintf(input_path, "%s_in.webp", tmp_base);
  sprintf(output_path, "%s_out.webp", tmp_base);

  GWPEncodeConfigInit(&temp_cfg);
  temp_cfg.lossless = GWP_TRUE;
  temp_cfg.exact = config->exact;
  temp_cfg.near_lossless = 100u;
  temp_cfg.output_buffer = (GWPu8*)config->scratch;
  temp_cfg.output_buffer_size = config->scratch_size;
  temp_cfg.scratch = 0;
  temp_cfg.scratch_size = 0u;
  if (GWPEncodePixels(pixels, width, height, stride, pixel_format, &temp_cfg, &temp_size) != GWP_STATUS_OK) {
    return GWP_STATUS_NOT_IMPLEMENTED;
  }
  if (!GWPWriteFileBridge(input_path, temp_cfg.output_buffer, temp_size)) {
    remove(input_path);
    remove(output_path);
    return GWP_STATUS_NOT_IMPLEMENTED;
  }

  cwebp = (config->cwebp_path != 0) ? config->cwebp_path : "cwebp";
  sprintf(preset_arg, "-preset %s", GWPPresetArg(config->preset));
  sprintf(filter_arg, "-f %u", (unsigned int)config->filter_strength);
  sprintf(alpha_q_arg, "-alpha_q %u", (unsigned int)config->alpha_quality);
  sprintf(command,
          "%s %s -q %u -m %u %s %s -alpha_filter %s %s %s -o \"%s\" \"%s\" > /dev/null 2>&1",
          cwebp,
          preset_arg,
          (unsigned int)config->quality,
          (unsigned int)config->method,
          filter_arg,
          alpha_q_arg,
          GWPAlphaFilterArg(config->alpha_filtering),
          config->use_sharp_yuv ? "-sharp_yuv" : "",
          config->exact ? "-exact" : "",
          output_path,
          input_path);
  rc = system(command);
  remove(input_path);
  if (rc != 0) {
    remove(output_path);
    return GWP_STATUS_NOT_IMPLEMENTED;
  }
  if (!GWPReadFileBridge(output_path, config->output_buffer,
                         config->output_buffer_size, out_size)) {
    remove(output_path);
    return GWP_STATUS_NOT_ENOUGH_OUTPUT;
  }
  remove(output_path);
  return GWP_STATUS_OK;
}
