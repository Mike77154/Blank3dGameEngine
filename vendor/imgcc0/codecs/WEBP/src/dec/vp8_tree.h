#ifndef GWP_DEC_VP8_TREE_H_
#define GWP_DEC_VP8_TREE_H_

#include "vp8_bool.h"

#ifdef __cplusplus
extern "C" {
#endif

GWPBool GWPVP8ReadTree(const int* tree,
                       const GWPu8* probs,
                       int tree_size,
                       GWPVP8BoolDecoder* br,
                       int* out_value);

GWPBool GWPVP8ReadTreeAt(const int* tree,
                         const GWPu8* probs,
                         int tree_size,
                         int start_index,
                         GWPVP8BoolDecoder* br,
                         int* out_value);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_TREE_H_ */
