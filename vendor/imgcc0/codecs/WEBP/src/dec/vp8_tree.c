#include "vp8_tree.h"

GWPBool GWPVP8ReadTreeAt(const int* tree,
                         const GWPu8* probs,
                         int tree_size,
                         int start_index,
                         GWPVP8BoolDecoder* br,
                         int* out_value) {
  int node;
  if (tree == 0 || probs == 0 || br == 0 || out_value == 0) return GWP_FALSE;
  if (tree_size < 2 || start_index < 0 || start_index >= tree_size) return GWP_FALSE;
  node = start_index;
  for (;;) {
    GWPu32 bit;
    int next_index;
    int prob_index;
    if ((node & 1) != 0) return GWP_FALSE;
    if (node + 1 >= tree_size) return GWP_FALSE;
    prob_index = node >> 1;
    if (!GWPVP8BoolGet(br, (int)probs[prob_index], &bit)) return GWP_FALSE;
    next_index = tree[node + (int)bit];
    if (next_index <= 0) {
      *out_value = -next_index;
      return GWP_TRUE;
    }
    if (next_index >= tree_size) return GWP_FALSE;
    node = next_index;
  }
}

GWPBool GWPVP8ReadTree(const int* tree,
                       const GWPu8* probs,
                       int tree_size,
                       GWPVP8BoolDecoder* br,
                       int* out_value) {
  return GWPVP8ReadTreeAt(tree, probs, tree_size, 0, br, out_value);
}
