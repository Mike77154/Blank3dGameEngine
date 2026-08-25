#ifndef GWP_ENC_HUFFMAN_ENC_H_
#define GWP_ENC_HUFFMAN_ENC_H_

#include "../webp/types.h"
#include "bit_writer.h"

#define GWP_ENC_MAX_ALPHABET GWP_VP8L_MAX_GREEN_ALPHABET

typedef struct GWPHuffEncSymbol {
  GWPu16 code;
  GWPu8 bits;
} GWPHuffEncSymbol;

GWPStatusCode GWPHuffEncBuildLengths(const GWPu32* freqs,
                                     GWPu32 alphabet_size,
                                     GWPu8* lengths);

void GWPHuffEncBuildCodes(const GWPu8* lengths,
                          GWPu32 alphabet_size,
                          GWPHuffEncSymbol* symbols);

void GWPHuffEncPutSymbol(const GWPHuffEncSymbol* symbols,
                         GWPu32 symbol,
                         GWPBitWriter* bw);

#endif  /* GWP_ENC_HUFFMAN_ENC_H_ */
