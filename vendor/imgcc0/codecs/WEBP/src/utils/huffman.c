#include "huffman.h"

static GWPu32 GWPNextKey(GWPu32 key, int len) {
  GWPu32 step;
  step = ((GWPu32)1u) << (len - 1);
  while ((key & step) != 0u) {
    step >>= 1;
  }
  if (step == 0u) return key;
  return (key & (step - 1u)) + step;
}

static void GWPReplicateCode(GWPHuffCode* table,
                             int step,
                             int end,
                             GWPHuffCode code) {
  int cur;
  cur = end;
  while (cur > 0) {
    cur -= step;
    table[cur] = code;
  }
}

static int GWPNextTableBits(const GWPu32* count, int len, int root_bits) {
  int left;
  left = 1 << (len - root_bits);
  while (len < (int)GWP_VP8L_MAX_HUFF_BITS) {
    left -= (int)count[len];
    if (left <= 0) break;
    ++len;
    left <<= 1;
  }
  return len - root_bits;
}

static GWPu32 GWPBuildHuffmanTableInternal(GWPHuffCode* root_table,
                                           int root_bits,
                                           const GWPu8* lengths,
                                           GWPu32 num_symbols,
                                           GWPu16* sorted) {
  GWPu32 count[GWP_VP8L_MAX_HUFF_BITS + 1u];
  GWPu32 offset[GWP_VP8L_MAX_HUFF_BITS + 1u];
  GWPu32 total_size;
  GWPu32 i;
  GWPu32 symbol;

  if (lengths == 0 || num_symbols == 0u) return 0u;
  for (i = 0u; i <= GWP_VP8L_MAX_HUFF_BITS; ++i) {
    count[i] = 0u;
    offset[i] = 0u;
  }

  for (symbol = 0u; symbol < num_symbols; ++symbol) {
    if (lengths[symbol] > GWP_VP8L_MAX_HUFF_BITS) return 0u;
    ++count[lengths[symbol]];
  }
  if (count[0] == num_symbols) return 0u;

  total_size = ((GWPu32)1u) << root_bits;
  offset[1] = 0u;
  for (i = 1u; i < GWP_VP8L_MAX_HUFF_BITS; ++i) {
    if (count[i] > (((GWPu32)1u) << i)) return 0u;
    offset[i + 1u] = offset[i] + count[i];
  }

  for (symbol = 0u; symbol < num_symbols; ++symbol) {
    GWPu8 len;
    len = lengths[symbol];
    if (len != 0u) {
      if (sorted != 0) {
        if (offset[len] >= num_symbols) return 0u;
        sorted[offset[len]++] = (GWPu16)symbol;
      } else {
        ++offset[len];
      }
    }
  }

  if (offset[GWP_VP8L_MAX_HUFF_BITS] == 1u) {
    if (root_table != 0 && sorted != 0) {
      GWPHuffCode code;
      code.bits = 0u;
      code.value = sorted[0];
      GWPReplicateCode(root_table, 1, (int)total_size, code);
    }
    return total_size;
  }

  {
    int step;
    GWPu32 low;
    GWPu32 mask;
    GWPu32 key;
    int num_nodes;
    int num_open;
    int table_bits;
    int table_size;
    GWPu32 sorted_index;
    GWPHuffCode* table;

    low = 0xffffffffu;
    mask = total_size - 1u;
    key = 0u;
    num_nodes = 1;
    num_open = 1;
    table_bits = root_bits;
    table_size = 1 << table_bits;
    sorted_index = 0u;
    table = root_table;

    for (i = 1u, step = 2; i <= (GWPu32)root_bits; ++i, step <<= 1) {
      num_open <<= 1;
      num_nodes += num_open;
      num_open -= (int)count[i];
      if (num_open < 0) return 0u;
      if (table != 0 && sorted != 0) {
        while (count[i] > 0u) {
          GWPHuffCode code;
          code.bits = (GWPu8)i;
          code.value = sorted[sorted_index++];
          GWPReplicateCode(&table[key], step, table_size, code);
          key = GWPNextKey(key, (int)i);
          --count[i];
        }
      }
    }

    for (i = (GWPu32)(root_bits + 1), step = 2;
         i <= GWP_VP8L_MAX_HUFF_BITS;
         ++i, step <<= 1) {
      num_open <<= 1;
      num_nodes += num_open;
      num_open -= (int)count[i];
      if (num_open < 0) return 0u;
      while (count[i] > 0u) {
        GWPHuffCode code;
        if ((key & mask) != low) {
          int next_bits;
          if (table != 0 && sorted != 0) table += table_size;
          next_bits = GWPNextTableBits(count, (int)i, root_bits);
          table_bits = next_bits;
          table_size = 1 << table_bits;
          total_size += (GWPu32)table_size;
          low = key & mask;
          if (root_table != 0 && sorted != 0) {
            root_table[low].bits = (GWPu8)(table_bits + root_bits);
            root_table[low].value = (GWPu16)((table - root_table) - (int)low);
          }
        }
        if (table != 0 && sorted != 0) {
          code.bits = (GWPu8)(i - (GWPu32)root_bits);
          code.value = sorted[sorted_index++];
          GWPReplicateCode(&table[key >> root_bits], step, table_size, code);
        }
        key = GWPNextKey(key, (int)i);
        --count[i];
      }
    }

    if (num_nodes != 2 * (int)offset[GWP_VP8L_MAX_HUFF_BITS] - 1) {
      return 0u;
    }
  }

  return total_size;
}

void GWPHuffmanInit(GWPHuffman* huff,
                    GWPHuffCode* table,
                    GWPu32 capacity,
                    int root_bits) {
  if (huff == 0) return;
  huff->table = table;
  huff->capacity = capacity;
  huff->table_size = 0u;
  huff->root_bits = root_bits;
}

GWPStatusCode GWPHuffmanBuild(GWPHuffman* huff,
                              const GWPu8* lengths,
                              GWPu32 num_symbols) {
  GWPu16 sorted[GWP_VP8L_MAX_GREEN_ALPHABET];
  GWPu32 total_size;
  if (huff == 0 || lengths == 0 || huff->table == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (num_symbols > GWP_VP8L_MAX_GREEN_ALPHABET) {
    return GWP_STATUS_LIMIT_EXCEEDED;
  }
  total_size = GWPBuildHuffmanTableInternal(0, huff->root_bits, lengths, num_symbols, 0);
  if (total_size == 0u) return GWP_STATUS_BITSTREAM_ERROR;
  if (total_size > huff->capacity) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  huff->table_size = total_size;
  if (GWPBuildHuffmanTableInternal(huff->table, huff->root_bits, lengths,
                                   num_symbols, sorted) != total_size) {
    return GWP_STATUS_BITSTREAM_ERROR;
  }
  return GWP_STATUS_OK;
}

GWPStatusCode GWPHuffmanBuildArena(GWPHuffman* huff,
                                   GWPArena* arena,
                                   const GWPu8* lengths,
                                   GWPu32 num_symbols,
                                   int root_bits) {
  GWPHuffCode* table;
  GWPu32 total_size;
  if (huff == 0 || arena == 0 || lengths == 0) return GWP_STATUS_INVALID_PARAM;
  total_size = GWPBuildHuffmanTableInternal(0, root_bits, lengths, num_symbols, 0);
  if (total_size == 0u) return GWP_STATUS_BITSTREAM_ERROR;
  table = (GWPHuffCode*)GWPArenaAlloc(arena,
                                      total_size * (GWPu32)sizeof(*table),
                                      4u);
  if (table == 0) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  GWPHuffmanInit(huff, table, total_size, root_bits);
  return GWPHuffmanBuild(huff, lengths, num_symbols);
}

GWPStatusCode GWPHuffmanReadSymbol(const GWPHuffman* huff,
                                   GWPBitReader* br,
                                   GWPu32* out_symbol) {
  GWPu32 bits;
  GWPu32 mask;
  const GWPHuffCode* entry;
  int nbits;
  if (huff == 0 || br == 0 || out_symbol == 0 || huff->table == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (!GWPBitReaderPeekBits(br, huff->root_bits, &bits)) {
    return GWP_STATUS_TRUNCATED_DATA;
  }
  mask = (((GWPu32)1u) << huff->root_bits) - 1u;
  entry = &huff->table[bits & mask];
  nbits = (int)entry->bits - huff->root_bits;
  if (nbits > 0) {
    if (!GWPBitReaderSkipBits(br, huff->root_bits)) {
      return GWP_STATUS_TRUNCATED_DATA;
    }
    if (!GWPBitReaderPeekBits(br, nbits, &bits)) {
      return GWP_STATUS_TRUNCATED_DATA;
    }
    entry = entry + entry->value + bits;
    if (!GWPBitReaderSkipBits(br, (int)entry->bits)) {
      return GWP_STATUS_TRUNCATED_DATA;
    }
  } else {
    if (!GWPBitReaderSkipBits(br, (int)entry->bits)) {
      return GWP_STATUS_TRUNCATED_DATA;
    }
  }
  *out_symbol = entry->value;
  return GWP_STATUS_OK;
}
