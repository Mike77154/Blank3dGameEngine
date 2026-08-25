#ifndef ZRAGF_PROTOCOL89_MEM_H_INCLUDED
#define ZRAGF_PROTOCOL89_MEM_H_INCLUDED

#include "zragflib.h"

#ifndef ZRAGF_P89_STATIC_BYTES
#define ZRAGF_P89_STATIC_BYTES (64u * 1024u * 1024u)
#endif

void *zragf_p89_take(zragf_size_t bytes);
void *zragf_p89_resize(void *ptr, zragf_size_t old_bytes, zragf_size_t new_bytes);
void zragf_p89_release(void *ptr);
void zragf_p89_reset(void);
zragf_size_t zragf_p89_used(void);
zragf_size_t zragf_p89_capacity(void);

#endif
