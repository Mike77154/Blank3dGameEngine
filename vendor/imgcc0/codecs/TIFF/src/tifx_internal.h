/* SPDX-License-Identifier: CC0-1.0 */
#ifndef TIFX_INTERNAL_H
#define TIFX_INTERNAL_H

#include "../include/tifx.h"

int tifx__params_is_tiled(const tifx_write_params *params);
int tifx__compute_tile_count(unsigned long width,
                             unsigned long height,
                             unsigned long tile_width,
                             unsigned long tile_length,
                             unsigned long *out_tile_count);

int tifx__build_strip_byte_counts(const tifx_write_params *params,
                                  unsigned long stride_min,
                                  unsigned long rows_per_strip,
                                  unsigned long strip_count,
                                  unsigned long *strip_byte_counts,
                                  unsigned long *out_image_size);

int tifx__write_segment_payload(unsigned char *dst,
                                unsigned long dst_size,
                                const tifx_write_params *params,
                                unsigned long stride_min,
                                unsigned long segment_index,
                                unsigned long segment_span);

int tifx__write_strip_payload(unsigned char *dst,
                              unsigned long dst_size,
                              const tifx_write_params *params,
                              unsigned long stride_min,
                              unsigned long segment_index,
                              unsigned long start_row,
                              unsigned long row_count);


#endif
