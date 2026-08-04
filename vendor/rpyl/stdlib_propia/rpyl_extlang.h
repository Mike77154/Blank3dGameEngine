#ifndef RPYL_EXTLANG_H
#define RPYL_EXTLANG_H

/* RPYL Extension Module: init <lang>: */

#include "rpyl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpylLangHost RpylLangHost;

typedef int (*RpylInitLangFn)(
    RpylContext* ctx,
    void* userdata,
    const char* lang,
    int priority,
    const char* code,
    const char* filename,
    int start_line
);

RpylLangHost* rpyl_langhost_create(void);
void rpyl_langhost_destroy(RpylLangHost* host);

int rpyl_langhost_register(
    RpylLangHost* host,
    const char* lang,
    RpylInitLangFn fn,
    void* userdata
);

int rpyl_load_buffer_extlang(
    RpylContext* ctx,
    RpylLangHost* host,
    const char* buffer,
    size_t len,
    const char* filename
);

int rpyl_load_file_extlang(
    RpylContext* ctx,
    RpylLangHost* host,
    const char* path
);

#ifdef __cplusplus
}
#endif

#endif /* RPYL_EXTLANG_H */
