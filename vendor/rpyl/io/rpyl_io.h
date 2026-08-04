#ifndef RPYL_IO_H
#define RPYL_IO_H

#include <stddef.h>
#include "rpyl_stream.h"
#include "rpyl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpylIoBuffer {
    const char* data;
    size_t size;
    size_t pos;
} RpylIoBuffer;

void rpyl_io_buffer_init(RpylIoBuffer* b, const char* data, size_t size);
size_t rpyl_io_buffer_read(RpylIoBuffer* b, void* dst, size_t bytes);
int rpyl_io_buffer_getc(void* user);
int rpyl_io_buffer_ungetc(int c, void* user);
int rpyl_io_buffer_stream(RpylIoBuffer* b, RpylStream* out_stream);

typedef int (*RpylIoGetcFn)(void* user);
typedef int (*RpylIoUngetcFn)(int c, void* user);

typedef struct RpylIoSource {
    void* user;
    RpylIoGetcFn getc_fn;
    RpylIoUngetcFn ungetc_fn;
    unsigned long line;
    unsigned long column;
    unsigned long offset;
    int last_char;
} RpylIoSource;

void rpyl_io_source_init(RpylIoSource* src, void* user, RpylIoGetcFn getc_fn, RpylIoUngetcFn ungetc_fn);
int rpyl_io_source_getc(RpylIoSource* src);
int rpyl_io_source_ungetc(RpylIoSource* src, int c);
int rpyl_io_source_peek(RpylIoSource* src);
size_t rpyl_io_source_read_line(RpylIoSource* src, char* out, size_t out_size);
int rpyl_io_source_stream(RpylIoSource* src, RpylStream* out_stream);

typedef size_t (*RpylIoReadFn)(void* user, void* handle, void* dst, size_t bytes);

typedef struct RpylIoReadAdapter {
    void* user;
    void* handle;
    RpylIoReadFn read_fn;
    unsigned char buf[RPYL_IO_VFS_CHUNK];
    size_t pos;
    size_t len;
    int has_push;
    int push;
} RpylIoReadAdapter;

void rpyl_io_read_adapter_init(RpylIoReadAdapter* a, void* user, void* handle, RpylIoReadFn read_fn);
int rpyl_io_read_adapter_getc(void* user);
int rpyl_io_read_adapter_ungetc(int c, void* user);

#ifdef __cplusplus
}
#endif

#endif
