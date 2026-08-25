#ifndef GWP_ENC_VP8_TREE_ENC_H_
#define GWP_ENC_VP8_TREE_ENC_H_

#include "vp8_bool_enc.h"
#include "../webp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

GWPBool GWPVP8WriteTreeValueAt(GWPVP8BoolEncoder* bw,
                               const int* tree,
                               const GWPu8* probs,
                               int tree_size,
                               int start_index,
                               int value);

GWPBool GWPVP8WriteTreeValue(GWPVP8BoolEncoder* bw,
                             const int* tree,
                             const GWPu8* probs,
                             int tree_size,
                             int value);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_ENC_VP8_TREE_ENC_H_ */
