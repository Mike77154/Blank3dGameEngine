#ifndef GWP_UTILS_HUFFMAN_H_
#define GWP_UTILS_HUFFMAN_H_

#include "../webp/types.h"
#include "arena.h"
#include "bit_reader.h"
#include "common.h"

#define GWP_HUFFMAN_ROOT_BITS 8

typedef struct GWPHuffCode {
  GWPu8 bits;
  GWPu16 value;
} GWPHuffCode;

typedef struct GWPHuffman {
  GWPHuffCode* table;
  GWPu32 capacity;
  GWPu32 table_size;
  int root_bits;
} GWPHuffman;

void GWPHuffmanInit(GWPHuffman* huff,
                    GWPHuffCode* table,
                    GWPu32 capacity,
                    int root_bits);

GWPStatusCode GWPHuffmanBuild(GWPHuffman* huff,
                              const GWPu8* lengths,
                              GWPu32 num_symbols);

GWPStatusCode GWPHuffmanBuildArena(GWPHuffman* huff,
                                   GWPArena* arena,
                                   const GWPu8* lengths,
                                   GWPu32 num_symbols,
                                   int root_bits);

GWPStatusCode GWPHuffmanReadSymbol(const GWPHuffman* huff,
                                   GWPBitReader* br,
                                   GWPu32* out_symbol);

#endif  /* GWP_UTILS_HUFFMAN_H_ */
