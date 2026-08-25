#include <stdio.h>
#include <stdlib.h>

#include "../src/webp/decode.h"

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

static int WritePAM(const char* path, const unsigned char* rgba, unsigned int w, unsigned int h) {
  FILE* f;
  unsigned int y;
  f = fopen(path, "wb");
  if (f == 0) return 0;
  fprintf(f, "P7\nWIDTH %u\nHEIGHT %u\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", w, h);
  for (y = 0u; y < h; ++y) {
    if (fwrite(rgba + y * w * 4u, 1, w * 4u, f) != w * 4u) {
      fclose(f);
      return 0;
    }
  }
  fclose(f);
  return 1;
}

int main(int argc, char** argv) {
  unsigned char* data;
  long file_size;
  GWPBitstreamFeatures features;
  GWPDecoderOptions opt;
  unsigned char* rgba;
  unsigned char* scratch;
  unsigned int rgba_size;
  unsigned int scratch_size;
  GWPStatusCode st;

  if (argc != 3) {
    fprintf(stderr, "uso: %s image.webp out.pam\n", argv[0]);
    return 1;
  }

  data = 0;
  file_size = ReadFile(argv[1], &data);
  if (file_size < 0) {
    fprintf(stderr, "no pude leer %s\n", argv[1]);
    return 1;
  }

  st = GWPGetFeatures(data, (GWPu32)file_size, &features);
  if (st != GWP_STATUS_OK) {
    fprintf(stderr, "features error: %s\n", GWPStatusToString(st));
    free(data);
    return 1;
  }

  rgba_size = features.width * features.height * 4u;
  scratch_size = rgba_size * 8u + 1u * 1024u * 1024u;
  rgba = (unsigned char*)malloc(rgba_size);
  scratch = (unsigned char*)malloc(scratch_size);
  if (rgba == 0 || scratch == 0) {
    fprintf(stderr, "memoria insuficiente\n");
    free(rgba);
    free(scratch);
    free(data);
    return 1;
  }

  opt.pixel_format = GWP_PIXFMT_RGBA;
  opt.strict = 1;
  opt.max_width = 16384u;
  opt.max_height = 16384u;
  opt.output_buffer = rgba;
  opt.output_buffer_size = rgba_size;
  opt.output_stride = features.width * 4u;
  opt.scratch = scratch;
  opt.scratch_size = scratch_size;

  st = GWPDecode(data, (GWPu32)file_size, &opt, &features);
  if (st != GWP_STATUS_OK) {
    fprintf(stderr, "decode error: %s\n", GWPStatusToString(st));
    free(rgba);
    free(scratch);
    free(data);
    return 1;
  }

  if (!WritePAM(argv[2], rgba, features.width, features.height)) {
    fprintf(stderr, "no pude escribir %s\n", argv[2]);
    free(rgba);
    free(scratch);
    free(data);
    return 1;
  }

  free(rgba);
  free(scratch);
  free(data);
  return 0;
}
