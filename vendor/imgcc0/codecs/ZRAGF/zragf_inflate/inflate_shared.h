#ifndef ZRAGF_INFLATE_SHARED_H_INCLUDED
#define ZRAGF_INFLATE_SHARED_H_INCLUDED

#include "../zragflib_internal.h"

#define ZRAGF_INF_MAX_WINDOW 32768
#define ZRAGF_INF_GZIP_META_LIMIT ((zragf_size_t)1048576u)

extern const int zragf_inf_cl_order[19];
extern const int zragf_inf_len_base[29];
extern const int zragf_inf_len_extra[29];
extern const int zragf_inf_dist_base[30];
extern const int zragf_inf_dist_extra[30];

void zragf_inflate_window_put_byte(zragf_u8 *window,
                                   int window_size,
                                   int *window_pos,
                                   zragf_size_t *window_filled,
                                   zragf_u8 value);

int zragf_inflate_distance_ok(zragf_size_t window_filled,
                              int window_size,
                              int dist);

int zragf_inflate_length_symbol_info(int sym,
                                     int *base,
                                     int *extra_bits);

int zragf_inflate_distance_symbol_info(int sym,
                                       int *base,
                                       int *extra_bits);

int zragf_inflate_prepare_dist_lengths(int *dist_len,
                                       int count,
                                       int *dist_table_dummy);

int zragf_inflate_gzip_meta_limit_ok(zragf_size_t used);

#endif /* ZRAGF_INFLATE_SHARED_H_INCLUDED */
