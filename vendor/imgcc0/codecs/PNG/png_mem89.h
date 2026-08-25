#ifndef PNG_MEM89_H
#define PNG_MEM89_H

#ifndef PNG_MEM89_ARENA_BYTES
#define PNG_MEM89_ARENA_BYTES (64u * 1024u * 1024u)
#endif

void png_mem89_reset(void);
void* png_mem89_alloc(unsigned int bytes);
void* png_mem89_alloc_zero(unsigned int count, unsigned int bytes_each);
void* png_mem89_resize(void* ptr, unsigned int new_bytes);
void png_mem89_release(void* ptr);
unsigned int png_mem89_capacity(void);
unsigned int png_mem89_bytes_used(void);
unsigned int png_mem89_peak_used(void);

#endif
