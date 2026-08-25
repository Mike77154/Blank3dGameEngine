/* png_filters.c - filtros PNG (Sub/Up/Average/Paeth) + SIMD opcional
 *
 * Depende de png_decoder.h para tipos y defines de SSE2/NEON.
 */

#include "png_decoder.h"

/* --------------------------------------------------------- */
/*   NUEVO: modo SAFE / FAST para el desfiltrado             */
/* --------------------------------------------------------- */

typedef enum png_unfilter_mode_e
{
    PNG_UNFILTER_MODE_SAFE = 0,  /* checks básicos de entrada */
    PNG_UNFILTER_MODE_FAST = 1   /* asume entradas válidas */
} png_unfilter_mode;

/* Modo por defecto para png_unfilter_scanline (mantiene compat) */
#ifndef PNG_UNFILTER_DEFAULT_MODE
#define PNG_UNFILTER_DEFAULT_MODE PNG_UNFILTER_MODE_SAFE
#endif

/* API extendida (puedes declarar el prototipo en png_decoder.h si quieres):
 *
 *   void png_unfilter_scanline_mode(png_u8* row,
 *                                   const png_u8* prev_row,
 *                                   png_u32 rowbytes,
 *                                   int filter_type,
 *                                   png_u32 bpp,
 *                                   png_unfilter_mode mode);
 */

/* SIMD opcional para filtro Up */
#if PNG_DEC_ENABLE_SSE2
#include <emmintrin.h>
#endif

#if PNG_DEC_ENABLE_NEON
#include <arm_neon.h>
#endif

/* Paeth predictor (exportado) */
int png_paeth_predictor(int a, int b, int c)
{
    int p, pa, pb, pc;

    p  = a + b - c;
    pa = p > a ? p - a : a - p;
    pb = p > b ? p - b : b - p;
    pc = p > c ? p - c : c - p;

    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

/* SIMD para filtro Up (opcional) */
#if PNG_DEC_ENABLE_SSE2
static void png_unfilter_up_sse2(png_u8* row,
                                 const png_u8* prev_row,
                                 png_u32 rowbytes)
{
    png_u32 i = 0;
    for (i = 0; i + 16u <= rowbytes; i += 16u)
    {
        __m128i a = _mm_loadu_si128((const __m128i*)(row + i));
        __m128i b = _mm_loadu_si128((const __m128i*)(prev_row + i));
        __m128i r = _mm_add_epi8(a, b); /* suma mod 256 */
        _mm_storeu_si128((__m128i*)(row + i), r);
    }
    for (; i < rowbytes; ++i)
    {
        row[i] = (png_u8)((row[i] + prev_row[i]) & 0xFF);
    }
}
#endif

#if PNG_DEC_ENABLE_NEON
static void png_unfilter_up_neon(png_u8* row,
                                 const png_u8* prev_row,
                                 png_u32 rowbytes)
{
    png_u32 i = 0;
    for (i = 0; i + 16u <= rowbytes; i += 16u)
    {
        uint8x16_t a = vld1q_u8(row + i);
        uint8x16_t b = vld1q_u8(prev_row + i);
        uint8x16_t r = vaddq_u8(a, b); /* suma mod 256 */
        vst1q_u8(row + i, r);
    }
    for (; i < rowbytes; ++i)
    {
        row[i] = (png_u8)((row[i] + prev_row[i]) & 0xFF);
    }
}
#endif

/* --------------------------------------------------------- */
/*  NUEVO: implementación interna con parámetro "mode"       */
/* --------------------------------------------------------- */

static void png_unfilter_scanline_impl(png_u8* row,
                                       const png_u8* prev_row,
                                       png_u32 rowbytes,
                                       int filter_type,
                                       png_u32 bpp,
                                       png_unfilter_mode mode)
{
    png_u32 i;

    /* Modo SAFE: checks básicos de sanidad que no existían antes.
     * Modo FAST: asumimos que el caller pasó todo bien.
     * (No cambiamos el comportamiento en el camino "bueno").
     */
    if (mode == PNG_UNFILTER_MODE_SAFE)
    {
        if (!row || rowbytes == 0u)
            return;

        /* Filtros que usan "left" necesitan bpp > 0 */
        if ((filter_type == 1 || filter_type == 3 || filter_type == 4) &&
            bpp == 0u)
        {
            return;
        }

        /* Si el filtro requiere prev_row, pero no lo hay, mantenemos
         * el comportamiento anterior (no tocar la fila) simplemente
         * devolviendo aquí.
         */
        if (!prev_row &&
            (filter_type == 2 || filter_type == 3 || filter_type == 4))
        {
            return;
        }

        /* Filtros desconocidos: no hacemos nada (igual que antes) */
        if (filter_type < 0 || filter_type > 4)
            return;
    }

    /* A partir de aquí, FAST y SAFE comparten la misma lógica.
     * La diferencia es que SAFE llegó aquí sólo si pasó check básico.
     */

    switch (filter_type)
    {
        case 0: /* None */
            /* Nada que hacer */
            break;

        case 1: /* Sub */
            /* Depende de bpp, por eso no tocamos esta lógica */
            for (i = bpp; i < rowbytes; ++i)
            {
                row[i] = (png_u8)((row[i] + row[i - bpp]) & 0xFF);
            }
            break;

        case 2: /* Up */
            if (prev_row)
            {
#if PNG_DEC_ENABLE_SSE2
                /* En FAST podrías forzar siempre SSE2 si está disponible,
                 * pero como ya es muy rápido lo dejamos igual por claridad.
                 */
                png_unfilter_up_sse2(row, prev_row, rowbytes);
#elif PNG_DEC_ENABLE_NEON
                png_unfilter_up_neon(row, prev_row, rowbytes);
#else
                for (i = 0; i < rowbytes; ++i)
                {
                    row[i] = (png_u8)((row[i] + prev_row[i]) & 0xFF);
                }
#endif
            }
            /* Sin prev_row => en SAFE ya habríamos salido antes.
             * En FAST simplemente no tocamos la fila (igual que antes).
             */
            break;

        case 3: /* Average */
            if (prev_row)
            {
                for (i = 0; i < rowbytes; ++i)
                {
                    png_u8 left = (i >= bpp) ? row[i - bpp] : 0;
                    png_u8 up   = prev_row[i];
                    row[i] = (png_u8)((row[i] + ((left + up) >> 1)) & 0xFF);
                }
            }
            else
            {
                /* Caso sin prev_row (primera línea con filtro 3),
                 * mantenemos el comportamiento original.
                 */
                for (i = 0; i < rowbytes; ++i)
                {
                    png_u8 left = (i >= bpp) ? row[i - bpp] : 0;
                    row[i] = (png_u8)((row[i] + (left >> 1)) & 0xFF);
                }
            }
            break;

        case 4: /* Paeth */
            if (prev_row)
            {
                for (i = 0; i < rowbytes; ++i)
                {
                    png_u8 left   = (i >= bpp) ? row[i - bpp] : 0;
                    png_u8 up     = prev_row[i];
                    png_u8 upleft = (i >= bpp) ? prev_row[i - bpp] : 0;
                    int paeth     = png_paeth_predictor(left, up, upleft);
                    row[i] = (png_u8)((row[i] + paeth) & 0xFF);
                }
            }
            else
            {
                for (i = 0; i < rowbytes; ++i)
                {
                    png_u8 left = (i >= bpp) ? row[i - bpp] : 0;
                    int paeth = png_paeth_predictor(left, 0, 0);
                    row[i] = (png_u8)((row[i] + paeth) & 0xFF);
                }
            }
            break;

        default:
            /* Filtro desconocido, el caller puede tratarlo como error si quiere */
            break;
    }
}

/* --------------------------------------------------------- */
/*  API extendida + wrapper compatible                       */
/* --------------------------------------------------------- */

/* Nueva función: eliges SAFE o FAST desde el caller */
void png_unfilter_scanline_mode(png_u8* row,
                                const png_u8* prev_row,
                                png_u32 rowbytes,
                                int filter_type,
                                png_u32 bpp,
                                png_unfilter_mode mode)
{
    png_unfilter_scanline_impl(row, prev_row, rowbytes, filter_type, bpp, mode);
}

/* Desfiltrado de una scanline, in-place (exportado).
 * Conserva la firma original y el comportamiento previo,
 * usando el modo por defecto (SAFE).
 */
void png_unfilter_scanline(png_u8* row,
                           const png_u8* prev_row,
                           png_u32 rowbytes,
                           int filter_type,
                           png_u32 bpp)
{
    png_unfilter_scanline_impl(row,
                               prev_row,
                               rowbytes,
                               filter_type,
                               bpp,
                               PNG_UNFILTER_DEFAULT_MODE);
}
