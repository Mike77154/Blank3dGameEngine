#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/webp/anim_decode.h"
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

static int WritePAM(const char* path,
                    const unsigned char* rgba,
                    unsigned int w,
                    unsigned int h,
                    unsigned int stride) {
  FILE* f;
  unsigned int y;
  f = fopen(path, "wb");
  if (f == 0) return 0;
  fprintf(f, "P7\nWIDTH %u\nHEIGHT %u\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", w, h);
  for (y = 0u; y < h; ++y) {
    if (fwrite(rgba + y * stride, 1, w * 4u, f) != w * 4u) {
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
  GWPAnimDecoder dec;
  GWPAnimDecoderOptions opt;
  GWPAnimInfo info;
  GWPBitstreamFeatures features;
  unsigned char* scratch;
  unsigned char* rgba;
  unsigned int rgba_size;
  unsigned int scratch_size;
  unsigned int i;
  GWPStatusCode st;
  char path[1024];
  FILE* meta;
  char meta_path[1024];

  if (argc != 3) {
    fprintf(stderr, "uso: %s anim.webp out_prefix\n", argv[0]);
    return 1;
  }

  data = 0;
  file_size = ReadFile(argv[1], &data);
  if (file_size < 0) {
    fprintf(stderr, "no pude leer %s\n", argv[1]);
    return 1;
  }

  GWPAnimDecoderOptionsInit(&opt);
  st = GWPGetFeatures(data, (GWPu32)file_size, &features);
  if (st != GWP_STATUS_OK) {
    fprintf(stderr, "features error: %s\n", GWPStatusToString(st));
    free(data);
    return 1;
  }

  rgba_size = features.width * features.height * 4u;
  scratch_size = rgba_size * 16u + 1u * 1024u * 1024u;
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
  opt.scratch = scratch;
  opt.scratch_size = scratch_size;
  st = GWPAnimDecoderInit(&dec, data, (GWPu32)file_size, &opt);
  if (st != GWP_STATUS_OK) {
    fprintf(stderr, "anim init error: %s\n", GWPStatusToString(st));
    free(rgba);
    free(scratch);
    free(data);
    return 1;
  }
  st = GWPAnimDecoderGetInfo(&dec, &info);
  if (st != GWP_STATUS_OK) {
    fprintf(stderr, "anim info error: %s\n", GWPStatusToString(st));
    free(rgba);
    free(scratch);
    free(data);
    return 1;
  }

  sprintf(meta_path, "%s.jsonl", argv[2]);
  meta = fopen(meta_path, "w");
  if (meta == 0) {
    fprintf(stderr, "no pude escribir %s\n", meta_path);
    free(rgba);
    free(scratch);
    free(data);
    return 1;
  }

  i = 0u;
  while (GWPAnimDecoderHasMoreFrames(&dec)) {
    GWPu32 timestamp_ms;
    const GWPFrameInfo* frame;
    st = GWPAnimDecoderGetNext(&dec, rgba, rgba_size, info.canvas_width * 4u, &timestamp_ms);
    if (st != GWP_STATUS_OK) {
      fprintf(stderr, "anim next error: %s\n", GWPStatusToString(st));
      fclose(meta);
      free(rgba);
      free(scratch);
      free(data);
      return 1;
    }
    sprintf(path, "%s_%03u.pam", argv[2], i);
    if (!WritePAM(path, rgba, info.canvas_width, info.canvas_height, info.canvas_width * 4u)) {
      fprintf(stderr, "no pude escribir %s\n", path);
      fclose(meta);
      free(rgba);
      free(scratch);
      free(data);
      return 1;
    }
    frame = GWPDemuxGetFrame(&dec.demux, i);
    if (frame != 0) {
      fprintf(meta,
              "{\"index\":%u,\"timestamp_ms\":%u,\"duration_ms\":%u,\"x\":%u,\"y\":%u,\"width\":%u,\"height\":%u,\"blend\":%u,\"dispose\":%u}\n",
              i,
              timestamp_ms,
              frame->duration_ms,
              frame->x,
              frame->y,
              frame->width,
              frame->height,
              frame->blend_method,
              frame->dispose_method);
    }
    ++i;
  }

  fclose(meta);
  free(rgba);
  free(scratch);
  free(data);
  return 0;
}
