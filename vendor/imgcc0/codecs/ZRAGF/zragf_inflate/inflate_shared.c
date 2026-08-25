#include "inflate_shared.h"

const int zragf_inf_cl_order[19] = {
    16,17,18, 0, 8, 7, 9, 6,
    10, 5,11, 4,12, 3,13, 2,
    14, 1,15
};

const int zragf_inf_len_base[29] = {
    3, 4, 5, 6, 7, 8, 9,10,
   11,13,15,17,19,23,27,31,
   35,43,51,59,67,83,99,115,
  131,163,195,227,258
};

const int zragf_inf_len_extra[29] = {
    0,0,0,0,0,0,0,0,
    1,1,1,1,2,2,2,2,
    3,3,3,3,4,4,4,4,
    5,5,5,5,0
};

const int zragf_inf_dist_base[30] = {
     1,    2,    3,    4,
     5,    7,    9,   13,
    17,   25,   33,   49,
    65,   97,  129,  193,
   257,  385,  513,  769,
  1025, 1537, 2049, 3073,
  4097, 6145, 8193,12289,
 16385,24577
};

const int zragf_inf_dist_extra[30] = {
    0,0,0,0,1,1,2,2,
    3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,
   11,11,12,12,13,13
};

void zragf_inflate_window_put_byte(zragf_u8 *window,
                                   int window_size,
                                   int *window_pos,
                                   zragf_size_t *window_filled,
                                   zragf_u8 value)
{
    if (!window || window_size <= 0 || !window_pos || !window_filled)
        return;

    window[*window_pos] = value;
    *window_pos = (*window_pos + 1) % window_size;
    if (*window_filled < (zragf_size_t)window_size)
        (*window_filled)++;
}

int zragf_inflate_distance_ok(zragf_size_t window_filled,
                              int window_size,
                              int dist)
{
    zragf_size_t available = window_filled;

    if (dist <= 0 || window_size <= 0)
        return 0;
    if (dist > window_size)
        return 0;
    if (available > (zragf_size_t)window_size)
        available = (zragf_size_t)window_size;
    return (zragf_size_t)dist <= available;
}


int zragf_inflate_length_symbol_info(int sym,
                                     int *base,
                                     int *extra_bits)
{
    int idx;

    if (!base || !extra_bits)
        return 0;
    if (sym < 257 || sym > 285)
        return 0;

    idx = sym - 257;
    if (idx < 0 || idx >= 29)
        return 0;

    *base = zragf_inf_len_base[idx];
    *extra_bits = zragf_inf_len_extra[idx];
    return 1;
}

int zragf_inflate_distance_symbol_info(int sym,
                                       int *base,
                                       int *extra_bits)
{
    if (!base || !extra_bits)
        return 0;
    if (sym < 0 || sym >= 30)
        return 0;

    *base = zragf_inf_dist_base[sym];
    *extra_bits = zragf_inf_dist_extra[sym];
    return 1;
}

int zragf_inflate_prepare_dist_lengths(int *dist_len,
                                       int count,
                                       int *dist_table_dummy)
{
    int i;
    int non_zero = 0;

    if (!dist_len || count <= 0 || !dist_table_dummy)
        return 0;

    for (i = 0; i < count; ++i) {
        if (dist_len[i] > 0) {
            non_zero = 1;
            break;
        }
    }

    *dist_table_dummy = non_zero ? 0 : 1;
    if (!non_zero)
        dist_len[0] = 1;

    return 1;
}

int zragf_inflate_gzip_meta_limit_ok(zragf_size_t used)
{
    return used <= ZRAGF_INF_GZIP_META_LIMIT;
}
