#include "giff_internal.h"

void giff_lzw_dec_reset(giff_decoder* dec)
{
    giff_u32 i;

    if (dec == 0) {
        return;
    }

    if (dec->lzw_prefix != 0) {
        for (i = 0u; i < (giff_u32)GIFF_LZW_TABLE_SIZE; ++i) {
            dec->lzw_prefix[i] = 0u;
        }
    }

    if (dec->lzw_suffix != 0) {
        for (i = 0u; i < (giff_u32)GIFF_LZW_TABLE_SIZE; ++i) {
            dec->lzw_suffix[i] = (giff_u8)(i & 0xFFu);
        }
    }

    if (dec->lzw_stack != 0) {
        giff_mem_zero(dec->lzw_stack, (giff_u32)GIFF_LZW_TABLE_SIZE);
    }
}
