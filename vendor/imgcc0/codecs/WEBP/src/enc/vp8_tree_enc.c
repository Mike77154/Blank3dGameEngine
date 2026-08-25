#include "vp8_tree_enc.h"

static GWPBool GWPVP8FindTreePath(const int* tree,
                                  int tree_size,
                                  int node,
                                  int value,
                                  GWPu8* bits,
                                  int depth,
                                  int* out_depth) {
  int next0;
  int next1;
  if (tree == 0 || bits == 0 || out_depth == 0) return GWP_FALSE;
  if (node < 0 || node + 1 >= tree_size) return GWP_FALSE;
  next0 = tree[node + 0];
  bits[depth] = 0u;
  if (next0 <= 0) {
    if (-next0 == value) {
      *out_depth = depth + 1;
      return GWP_TRUE;
    }
  } else if (GWPVP8FindTreePath(tree, tree_size, next0, value, bits, depth + 1, out_depth)) {
    return GWP_TRUE;
  }
  next1 = tree[node + 1];
  bits[depth] = 1u;
  if (next1 <= 0) {
    if (-next1 == value) {
      *out_depth = depth + 1;
      return GWP_TRUE;
    }
  } else if (GWPVP8FindTreePath(tree, tree_size, next1, value, bits, depth + 1, out_depth)) {
    return GWP_TRUE;
  }
  return GWP_FALSE;
}

GWPBool GWPVP8WriteTreeValueAt(GWPVP8BoolEncoder* bw,
                               const int* tree,
                               const GWPu8* probs,
                               int tree_size,
                               int start_index,
                               int value) {
  GWPu8 bits[32];
  int depth;
  int i;
  int node;
  if (bw == 0 || tree == 0 || probs == 0) return GWP_FALSE;
  if (!GWPVP8FindTreePath(tree, tree_size, start_index, value, bits, 0, &depth)) {
    return GWP_FALSE;
  }
  node = start_index;
  for (i = 0; i < depth; ++i) {
    int prob_index;
    int next_index;
    prob_index = node >> 1;
    GWPVP8BoolEncWrite(bw, (int)probs[prob_index], (int)bits[i]);
    next_index = tree[node + (int)bits[i]];
    if (next_index <= 0) break;
    node = next_index;
  }
  return GWPVP8BoolEncOk(bw);
}

GWPBool GWPVP8WriteTreeValue(GWPVP8BoolEncoder* bw,
                             const int* tree,
                             const GWPu8* probs,
                             int tree_size,
                             int value) {
  return GWPVP8WriteTreeValueAt(bw, tree, probs, tree_size, 0, value);
}
