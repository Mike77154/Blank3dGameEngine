/* png_chunks.c */
#include <string.h> /* memcmp, memcpy */
#include "png_decoder_internal.h"
#include "png_chunks.h"

#ifndef PNG_FOURCC
#define PNG_FOURCC(a,b,c,d) \
    (((png_u32)(a) << 24) | ((png_u32)(b) << 16) | \
     ((png_u32)(c) <<  8) | ((png_u32)(d)))
#endif

static const png_u8 PNG_SIGNATURE[8] = {
    0x89, 'P','N','G', 0x0D,0x0A,0x1A,0x0A
};

static png_u32 png_read_be32(const png_u8* p)
{
    return ((png_u32)p[0] << 24) |
           ((png_u32)p[1] << 16) |
           ((png_u32)p[2] <<  8) |
           (png_u32)p[3];
}

static int png_is_valid_chunk_type(png_u32 t)
{
    int i;
    for (i = 0; i < 4; ++i) {
        png_u8 c = (png_u8)(t >> (24 - 8*i));
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
            return 0;
    }
    return 1;
}

int png_parse_chunks(const png_u8* data,
                     png_u32 size,
                     png_u32* io_pos,
                     png_state* st)
{
    png_u32 pos;
    int err;

    if (!data || !io_pos || !st)
        return PNG_DEC_ERR_FORMAT;

    err = PNG_DEC_OK;
    pos = *io_pos;

    if (size < 8u)
        return PNG_DEC_ERR_SIG;

    /* If caller starts at 0, validate signature and advance to first chunk. */
    if (pos == 0u) {
        if (memcmp(data, PNG_SIGNATURE, 8u) != 0)
            return PNG_DEC_ERR_SIG;
        pos = 8u;
    }

    /* If caller starts after signature, still validate signature once. */
    if (memcmp(data, PNG_SIGNATURE, 8u) != 0)
        return PNG_DEC_ERR_SIG;

    if (pos < 8u)
        return PNG_DEC_ERR_FORMAT;

    while (pos + 8u <= size) {
        png_u32 length;
        png_u32 type;
        png_u32 chunk_start;
        png_u32 next_pos;
        const png_u8* chunk_data;
        png_u32 crc_read;
        png_u32 crc_calc;

        length = png_read_be32(data + pos);
        type   = png_read_be32(data + pos + 4u);

        if (!png_is_valid_chunk_type(type)) {
            err = PNG_DEC_ERR_FORMAT;
            break;
        }

        chunk_start = pos + 8u;

        /* length + type + data + CRC must fit. PNG length is 31-bit by spec. */
        if (length > 0x7FFFFFFFu) {
            err = PNG_DEC_ERR_FORMAT;
            break;
        }

        if (length > st->max_chunk_bytes) {
            err = PNG_DEC_ERR_CHUNK_TOO_LARGE;
            break;
        }

        if (st->chunk_count >= st->max_chunks) {
            err = PNG_DEC_ERR_TOO_MANY_CHUNKS;
            break;
        }

        next_pos = chunk_start + length + 4u; /* +4 CRC */
        if (next_pos > size) {
            err = PNG_DEC_ERR_FORMAT;
            break;
        }

        chunk_data = data + chunk_start;

        /* CRC: type + data */
        crc_calc = png_crc32(data + pos + 4u, 4u + length);
        crc_read = png_read_be32(data + chunk_start + length);
        if (crc_calc != crc_read) {
            err = PNG_DEC_ERR_CRC;
            break;
        }

        st->chunk_count += 1u;
        err = png_handle_chunk(st, type, chunk_data, length);
        if (err != PNG_DEC_OK)
            break;

        pos = next_pos;

        /* IEND ends the PNG datastream. */
        if (type == PNG_FOURCC('I','E','N','D'))
            break;
    }

    *io_pos = pos;
    return err;
}

int png_handle_chunk(png_state* st,
                     png_u32 type,
                     const png_u8* d,
                     png_u32 length)
{
    png_u32 fourcc_IHDR = PNG_FOURCC('I','H','D','R');
    png_u32 fourcc_IDAT = PNG_FOURCC('I','D','A','T');
    png_u32 fourcc_IEND = PNG_FOURCC('I','E','N','D');
    png_u32 fourcc_dSIG = PNG_FOURCC('d','S','I','G');

    if (!st)
        return PNG_DEC_ERR_FORMAT;

    if (st->trailing_dsig_started && type != fourcc_dSIG && type != fourcc_IEND)
        return PNG_DEC_ERR_FORMAT;

    /* Once IDAT started, if we see any non-IDAT chunk, mark end of IDAT run. */
    if (st->seen_IDAT && !st->seen_IEND && type != fourcc_IDAT)
        st->idat_finished = 1;

    if (!st->seen_IHDR && type != fourcc_IHDR)
        return PNG_DEC_ERR_FORMAT;

    if (st->seen_IHDR && !st->seen_IDAT && !st->seen_IEND &&
        type != fourcc_IHDR && type != fourcc_dSIG)
        st->post_ihdr_non_dsig_seen = 1;

    if (type == fourcc_dSIG && st->seen_IDAT)
        st->trailing_dsig_started = 1;

    switch (type)
    {
        /* ---- Critical core ---- */
        case PNG_FOURCC('I','H','D','R'): return png_handle_IHDR(st,d,length);
        case PNG_FOURCC('P','L','T','E'): return png_handle_PLTE(st,d,length);
        case PNG_FOURCC('I','D','A','T'): return png_handle_IDAT(st,d,length);
        case PNG_FOURCC('I','E','N','D'): return png_handle_IEND(st,d,length);

        /* ---- Key ancillaries we actually use ---- */
        case PNG_FOURCC('t','R','N','S'): return png_handle_tRNS(st,d,length);
        case PNG_FOURCC('g','A','M','A'): return png_handle_gAMA(st,d,length);
        case PNG_FOURCC('c','H','R','M'): return png_handle_cHRM(st,d,length);
        case PNG_FOURCC('c','I','C','P'): return png_handle_cICP(st,d,length);
        case PNG_FOURCC('m','D','C','V'): return png_handle_mDCV(st,d,length);
        case PNG_FOURCC('c','L','L','I'): return png_handle_cLLI(st,d,length);
        case PNG_FOURCC('s','R','G','B'): return png_handle_sRGB(st,d,length);
        case PNG_FOURCC('p','H','Y','s'): return png_handle_pHYs(st,d,length);
        case PNG_FOURCC('b','K','G','D'): return png_handle_bKGD(st,d,length);
        case PNG_FOURCC('t','I','M','E'): return png_handle_tIME(st,d,length);
        case PNG_FOURCC('s','B','I','T'): return png_handle_sBIT(st,d,length);

        /* ---- Other ancillaries ---- */
        case PNG_FOURCC('t','E','X','t'): return png_handle_tEXt(st,d,length);
        case PNG_FOURCC('z','T','X','t'): return png_handle_zTXt(st,d,length);
        case PNG_FOURCC('i','T','X','t'): return png_handle_iTXt(st,d,length);
        case PNG_FOURCC('i','C','C','P'): return png_handle_iCCP(st,d,length);
        case PNG_FOURCC('e','X','I','f'): return png_handle_eXIf(st,d,length);
        case PNG_FOURCC('s','P','L','T'): return png_handle_sPLT(st,d,length);
        case PNG_FOURCC('h','I','S','T'): return png_handle_hIST(st,d,length);
        case PNG_FOURCC('o','F','F','s'): return png_handle_oFFs(st,d,length);
        case PNG_FOURCC('s','C','A','L'): return png_handle_sCAL(st,d,length);
        case PNG_FOURCC('p','C','A','L'): return png_handle_pCAL(st,d,length);
        case PNG_FOURCC('s','T','E','R'): return png_handle_sTER(st,d,length);
        case PNG_FOURCC('g','I','F','g'): return png_handle_gIFg(st,d,length);
        case PNG_FOURCC('g','I','F','x'): return png_handle_gIFx(st,d,length);
        case PNG_FOURCC('g','I','F','t'): return png_handle_gIFt(st,d,length);
        case PNG_FOURCC('d','S','I','G'): return png_handle_dSIG(st,d,length);
        case PNG_FOURCC('f','R','A','c'): return png_handle_fRAc(st,d,length);

        default:
        {
            png_u8 first;
            png_u8 last;
            int is_ancillary;
            png_u8 location;

            first = (png_u8)(type >> 24);
            last  = (png_u8)(type & 0xFFu);
            is_ancillary = (first & 0x20u) ? 1 : 0;

            if (!is_ancillary)
                return PNG_DEC_ERR_UNSUPPORTED;

            if (st->seen_IDAT)
                location = PNG_CHUNK_POS_AFTER_IDAT;
            else if (st->seen_PLTE)
                location = PNG_CHUNK_POS_AFTER_PLTE;
            else
                location = PNG_CHUNK_POS_AFTER_IHDR;

            if (st->keep_unknown_chunks == PNG_DEC_KEEP_UNKNOWN_ALL)
                return png_state_add_unknown_chunk(st, type, d, length, location);

            if (st->keep_unknown_chunks == PNG_DEC_KEEP_UNKNOWN_SAFE && (last & 0x20u))
                return png_state_add_unknown_chunk(st, type, d, length, location);

            return PNG_DEC_OK;
        }
    }
}
