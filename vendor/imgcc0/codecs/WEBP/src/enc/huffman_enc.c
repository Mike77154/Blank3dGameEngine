#include "huffman_enc.h"
#include "../utils/common.h"

static GWPu32 GWPNextKeyEnc(GWPu32 key, int len) {
  GWPu32 step;
  step = ((GWPu32)1u) << (len - 1);
  while ((key & step) != 0u) step >>= 1;
  if (step == 0u) return key;
  return (key & (step - 1u)) + step;
}

GWPStatusCode GWPHuffEncBuildLengths(const GWPu32* freqs,
                                     GWPu32 alphabet_size,
                                     GWPu8* lengths) {
  GWPu32 node_freq[2u * GWP_ENC_MAX_ALPHABET];
  int parent[2u * GWP_ENC_MAX_ALPHABET];
  GWPu32 leaf_symbol[GWP_ENC_MAX_ALPHABET];
  GWPu32 leaf_count;
  GWPu32 node_count;
  GWPu32 i;
  if (freqs == 0 || lengths == 0) return GWP_STATUS_INVALID_PARAM;
  if (alphabet_size == 0u || alphabet_size > GWP_ENC_MAX_ALPHABET) {
    return GWP_STATUS_LIMIT_EXCEEDED;
  }
  for (i = 0u; i < alphabet_size; ++i) lengths[i] = 0u;
  leaf_count = 0u;
  for (i = 0u; i < alphabet_size; ++i) {
    if (freqs[i] != 0u) {
      node_freq[leaf_count] = freqs[i];
      parent[leaf_count] = -1;
      leaf_symbol[leaf_count] = i;
      ++leaf_count;
    }
  }
  if (leaf_count == 0u) {
    lengths[0] = 1u;
    return GWP_STATUS_OK;
  }
  if (leaf_count == 1u) {
    lengths[leaf_symbol[0]] = 1u;
    return GWP_STATUS_OK;
  }
  node_count = leaf_count;
  while (node_count < 2u * leaf_count - 1u) {
    GWPu32 best0;
    GWPu32 best1;
    GWPu32 j;
    best0 = 0xffffffffu;
    best1 = 0xffffffffu;
    for (j = 0u; j < node_count; ++j) {
      if (parent[j] != -1) continue;
      if (best0 == 0xffffffffu || node_freq[j] < node_freq[best0] ||
          (node_freq[j] == node_freq[best0] && j < best0)) {
        best1 = best0;
        best0 = j;
      } else if (best1 == 0xffffffffu || node_freq[j] < node_freq[best1] ||
                 (node_freq[j] == node_freq[best1] && j < best1)) {
        best1 = j;
      }
    }
    if (best0 == 0xffffffffu || best1 == 0xffffffffu) {
      return GWP_STATUS_BITSTREAM_ERROR;
    }
    node_freq[node_count] = node_freq[best0] + node_freq[best1];
    parent[best0] = (int)node_count;
    parent[best1] = (int)node_count;
    parent[node_count] = -1;
    ++node_count;
  }
  for (i = 0u; i < leaf_count; ++i) {
    GWPu32 depth;
    int p;
    depth = 0u;
    p = parent[i];
    while (p != -1) {
      ++depth;
      p = parent[(GWPu32)p];
    }
    if (depth == 0u) depth = 1u;
    if (depth > GWP_VP8L_MAX_HUFF_BITS) return GWP_STATUS_LIMIT_EXCEEDED;
    lengths[leaf_symbol[i]] = (GWPu8)depth;
  }
  return GWP_STATUS_OK;
}

void GWPHuffEncBuildCodes(const GWPu8* lengths,
                          GWPu32 alphabet_size,
                          GWPHuffEncSymbol* symbols) {
  GWPu32 count[GWP_VP8L_MAX_HUFF_BITS + 1u];
  GWPu32 i;
  GWPu32 non_zero;
  GWPu32 single_symbol;
  GWPu32 key;
  int len;
  if (lengths == 0 || symbols == 0) return;
  for (i = 0u; i <= GWP_VP8L_MAX_HUFF_BITS; ++i) count[i] = 0u;
  non_zero = 0u;
  single_symbol = 0u;
  for (i = 0u; i < alphabet_size; ++i) {
    symbols[i].code = 0u;
    symbols[i].bits = lengths[i];
    if (lengths[i] != 0u) {
      ++count[lengths[i]];
      ++non_zero;
      single_symbol = i;
    }
  }
  if (non_zero == 1u) {
    symbols[single_symbol].bits = 0u;
    symbols[single_symbol].code = 0u;
    return;
  }
  key = 0u;
  for (len = 1; len <= (int)GWP_VP8L_MAX_HUFF_BITS; ++len) {
    for (i = 0u; i < alphabet_size; ++i) {
      if ((int)lengths[i] == len) {
        symbols[i].code = (GWPu16)key;
        symbols[i].bits = (GWPu8)len;
        key = GWPNextKeyEnc(key, len);
      }
    }
  }
}

void GWPHuffEncPutSymbol(const GWPHuffEncSymbol* symbols,
                         GWPu32 symbol,
                         GWPBitWriter* bw) {
  if (symbols == 0 || bw == 0) return;
  GWPBitWriterPutBits(bw, symbols[symbol].code, (int)symbols[symbol].bits);
}
