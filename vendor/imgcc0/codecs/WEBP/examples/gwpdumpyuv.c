#include <stdio.h>
#include <stdlib.h>

#include "../src/webp/demux.h"
#include "../src/webp/decode.h"
#include "../src/dec/vp8_dec.h"
#include "../src/dec/vp8_picture.h"

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

static int WriteFlatYUV(const char* path, const GWPVP8FrameBuffer* fb) {
  FILE* f;
  GWPu32 y;
  GWPu32 uv_w;
  GWPu32 uv_h;
  if (fb == 0) return 0;
  uv_w = (fb->width + 1u) >> 1;
  uv_h = (fb->height + 1u) >> 1;
  f = fopen(path, "wb");
  if (f == 0) return 0;
  for (y = 0u; y < fb->height; ++y) {
    if (fwrite(fb->y + y * fb->y_stride, 1, fb->width, f) != fb->width) {
      fclose(f);
      return 0;
    }
  }
  for (y = 0u; y < uv_h; ++y) {
    if (fwrite(fb->u + y * fb->uv_stride, 1, uv_w, f) != uv_w) {
      fclose(f);
      return 0;
    }
  }
  for (y = 0u; y < uv_h; ++y) {
    if (fwrite(fb->v + y * fb->uv_stride, 1, uv_w, f) != uv_w) {
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
  GWPDemuxer dmux;
  GWPDecoderOptions opt;
  GWPBitstreamFeatures features;
  GWPVP8ControlHeader control;
  GWPVP8FrameBuffer fb;
  GWPStatusCode st;
  unsigned char* scratch;
  unsigned int scratch_size;

  if (argc != 3) {
    fprintf(stderr, "uso: %s image.webp out.yuv\n", argv[0]);
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
    fprintf(stderr, "demux error: %s\n", GWPStatusToString(st));
    free(data);
    return 1;
  }
  if (dmux.features.format != GWP_BITSTREAM_VP8 || dmux.vp8_payload.bytes == 0) {
    fprintf(stderr, "solo soporta WebP VP8 lossy estatico\n");
    free(data);
    return 1;
  }

  scratch_size = dmux.features.width * dmux.features.height * 12u + (1u << 20);
  scratch = (unsigned char*)malloc(scratch_size);
  if (scratch == 0) {
    fprintf(stderr, "scratch insuficiente\n");
    free(data);
    return 1;
  }

  opt.pixel_format = GWP_PIXFMT_RGBA;
  opt.strict = 1;
  opt.max_width = 16384u;
  opt.max_height = 16384u;
  opt.output_buffer = 0;
  opt.output_buffer_size = 0u;
  opt.output_stride = 0u;
  opt.scratch = scratch;
  opt.scratch_size = scratch_size;

  st = GWPDecodeVP8WithState(dmux.vp8_payload.bytes,
                             dmux.vp8_payload.size,
                             &opt,
                             &features,
                             &control,
                             &fb);
  if (st != GWP_STATUS_OK) {
    fprintf(stderr, "vp8 decode error: %s\n", GWPStatusToString(st));
    free(scratch);
    free(data);
    return 1;
  }

  if (!WriteFlatYUV(argv[2], &fb)) {
    fprintf(stderr, "no pude escribir %s\n", argv[2]);
    free(scratch);
    free(data);
    return 1;
  }

  free(scratch);
  free(data);
  return 0;
}
