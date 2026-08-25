#include <stdio.h>
#include <stdlib.h>

#include "../src/webp/demux.h"
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

static const char* FourCCString(GWPu32 fourcc, char tmp[5]) {
  tmp[0] = (char)(fourcc & 0xffu);
  tmp[1] = (char)((fourcc >> 8) & 0xffu);
  tmp[2] = (char)((fourcc >> 16) & 0xffu);
  tmp[3] = (char)((fourcc >> 24) & 0xffu);
  tmp[4] = '\0';
  return tmp;
}

int main(int argc, char** argv) {
  unsigned char* data;
  long file_size;
  GWPDemuxer dmux;
  GWPStatusCode st;
  GWPu32 i;

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
    fprintf(stderr, "demux error: %s\n", GWPStatusToString(st));
    free(data);
    return 1;
  }

  printf("format: ");
  if (dmux.features.format == GWP_BITSTREAM_VP8L) printf("VP8L\n");
  else if (dmux.features.format == GWP_BITSTREAM_VP8) printf("VP8\n");
  else if (dmux.features.format == GWP_BITSTREAM_ANIMATION) printf("ANIMATION\n");
  else printf("UNKNOWN\n");

  printf("size: %u x %u\n", dmux.features.width, dmux.features.height);
  printf("alpha: %d\n", (int)dmux.features.has_alpha);
  printf("icc: %d\n", (int)dmux.features.has_icc);
  printf("exif: %d\n", (int)dmux.features.has_exif);
  printf("xmp: %d\n", (int)dmux.features.has_xmp);
  printf("animation: %d\n", (int)dmux.features.has_animation);
  printf("frames: %u\n", dmux.frame_count);
  if (dmux.features.has_animation) {
    printf("loop_count: %u\n", (unsigned int)dmux.loop_count);
    printf("background_bgra: 0x%08x\n", (unsigned int)dmux.background_color);
  }
  printf("chunks: %u\n", dmux.chunk_count);

  for (i = 0u; i < dmux.chunk_count; ++i) {
    char tag[5];
    printf("  [%03u] %s size=%u offset=%u\n",
           i,
           FourCCString(dmux.chunks[i].fourcc, tag),
           dmux.chunks[i].size,
           dmux.chunks[i].offset);
  }

  if (dmux.frame_count != 0u) {
    for (i = 0u; i < dmux.frame_count; ++i) {
      const GWPFrameInfo* f;
      char bit_tag[5];
      f = &dmux.frames[i];
      printf("frame %u: x=%u y=%u w=%u h=%u dur=%u blend=%u dispose=%u bits=%s alpha=%u\n",
             i,
             f->x,
             f->y,
             f->width,
             f->height,
             f->duration_ms,
             (unsigned int)f->blend_method,
             (unsigned int)f->dispose_method,
             FourCCString(f->bitstream_fourcc, bit_tag),
             (unsigned int)(f->alph_payload.bytes != 0));
    }
  }

  free(data);
  return 0;
}
