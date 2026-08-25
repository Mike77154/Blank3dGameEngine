#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/webp/anim_encode.h"

#define LOCAL_MAX_FRAMES 1024
#define LOCAL_LINE_SIZE  2048

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

static int WriteFile(const char* path, const unsigned char* data, size_t size) {
  FILE* f = fopen(path, "wb");
  if (f == 0) return 0;
  if (fwrite(data, 1, size, f) != size) {
    fclose(f);
    return 0;
  }
  fclose(f);
  return 1;
}

static int ParseU32(const char* text, unsigned int* out) {
  char* end;
  unsigned long v;
  if (text == 0 || out == 0) return 0;
  v = strtoul(text, &end, 0);
  if (end == text || *end != '\0') return 0;
  *out = (unsigned int)v;
  return 1;
}

static void Usage(const char* argv0) {
  fprintf(stderr,
          "uso: %s out.webp canvas_w canvas_h loop_count bgcolor manifest.tsv\n"
          "  manifest.tsv: path<TAB>duration_ms<TAB>x<TAB>y<TAB>blend<TAB>dispose\n"
          "  blend: 0=blend 1=no-blend\n"
          "  dispose: 0=none 1=background\n",
          argv0);
}

int main(int argc, char** argv) {
  unsigned int canvas_w, canvas_h, loop_count, bgcolor;
  const char* out_path;
  const char* manifest_path;
  FILE* mf;
  char line[LOCAL_LINE_SIZE];
  unsigned char* frame_bytes[LOCAL_MAX_FRAMES];
  unsigned int loaded_count;
  GWPAnimEncoder enc;
  GWPAnimEncoderOptions enc_options;
  GWPMuxAnimParams anim_params;
  GWPu32 out_size;
  GWPu32 estimate;
  unsigned char* out_data;
  int ok;

  if (argc != 7) {
    Usage(argv[0]);
    return 1;
  }
  out_path = argv[1];
  if (!ParseU32(argv[2], &canvas_w) || !ParseU32(argv[3], &canvas_h) ||
      !ParseU32(argv[4], &loop_count) || !ParseU32(argv[5], &bgcolor)) {
    fprintf(stderr, "argumentos numéricos inválidos\n");
    return 1;
  }
  manifest_path = argv[6];

  for (loaded_count = 0u; loaded_count < LOCAL_MAX_FRAMES; ++loaded_count) frame_bytes[loaded_count] = 0;
  loaded_count = 0u;
  out_data = 0;
  ok = 0;

  GWPAnimEncoderOptionsInit(&enc_options);
  GWPAnimEncoderInit(&enc, canvas_w, canvas_h, &enc_options);
  anim_params.bgcolor = bgcolor;
  anim_params.loop_count = (GWPu16)loop_count;
  if (GWPAnimEncoderSetAnimationParams(&enc, &anim_params) != GWP_MUX_OK) {
    fprintf(stderr, "no pude configurar ANIM\n");
    return 1;
  }

  mf = fopen(manifest_path, "rb");
  if (mf == 0) {
    fprintf(stderr, "no pude abrir %s\n", manifest_path);
    return 1;
  }

  while (fgets(line, (int)sizeof(line), mf) != 0) {
    char* tok;
    char* cols[6];
    unsigned int duration_ms, x, y, blend, dispose;
    unsigned char* bytes;
    long size;
    GWPData webp_data;
    GWPAnimFrameSpec spec;
    int col_count;

    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
    col_count = 0;
    tok = strtok(line, "\t\r\n");
    while (tok != 0 && col_count < 6) {
      cols[col_count++] = tok;
      tok = strtok(0, "\t\r\n");
    }
    if (col_count != 6) {
      fprintf(stderr, "línea inválida en manifest, esperaba 6 columnas\n");
      goto End;
    }
    if (!ParseU32(cols[1], &duration_ms) || !ParseU32(cols[2], &x) || !ParseU32(cols[3], &y) ||
        !ParseU32(cols[4], &blend) || !ParseU32(cols[5], &dispose)) {
      fprintf(stderr, "números inválidos en manifest\n");
      goto End;
    }
    if (loaded_count >= LOCAL_MAX_FRAMES) {
      fprintf(stderr, "demasiados frames en manifest\n");
      goto End;
    }
    bytes = 0;
    size = ReadFile(cols[0], &bytes);
    if (size < 0) {
      fprintf(stderr, "no pude leer frame %s\n", cols[0]);
      goto End;
    }
    frame_bytes[loaded_count] = bytes;
    ++loaded_count;

    webp_data.bytes = bytes;
    webp_data.size = (GWPu32)size;
    GWPAnimFrameSpecInit(&spec);
    spec.duration_ms = duration_ms;
    spec.x_offset = x;
    spec.y_offset = y;
    spec.blend_method = (GWPu8)blend;
    spec.dispose_method = (GWPu8)dispose;
    if (GWPAnimEncoderAddFrameWebP(&enc, &webp_data, &spec) != GWP_MUX_OK) {
      fprintf(stderr, "no pude agregar frame %s\n", cols[0]);
      goto End;
    }
  }
  fclose(mf);
  mf = 0;

  estimate = GWPAnimEncoderEstimateSize(&enc);
  if (estimate == 0u) {
    fprintf(stderr, "estimate_size devolvió 0\n");
    goto End;
  }
  out_data = (unsigned char*)malloc((size_t)estimate);
  if (out_data == 0) {
    fprintf(stderr, "sin memoria para output\n");
    goto End;
  }
  if (GWPAnimEncoderAssemble(&enc, out_data, estimate, &out_size) != GWP_MUX_OK) {
    fprintf(stderr, "falló assemble de animación\n");
    goto End;
  }
  if (!WriteFile(out_path, out_data, out_size)) {
    fprintf(stderr, "no pude escribir %s\n", out_path);
    goto End;
  }

  printf("ok: %u frame(s), %u bytes\n", (unsigned int)enc.frame_count, (unsigned int)out_size);
  ok = 1;

End:
  if (mf != 0) fclose(mf);
  if (out_data != 0) free(out_data);
  for (loaded_count = 0u; loaded_count < LOCAL_MAX_FRAMES; ++loaded_count) {
    if (frame_bytes[loaded_count] != 0) free(frame_bytes[loaded_count]);
  }
  return ok ? 0 : 1;
}
