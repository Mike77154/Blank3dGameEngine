#ifndef PNG_CHUNKS_H
#define PNG_CHUNKS_H

#include "png_decoder_internal.h"

/* Parser principal */
int png_parse_chunks(const png_u8* data,
                     png_u32 size,
                     png_u32* io_pos,
                     png_state* st);

/* Punto central para manejar chunks */
int png_handle_chunk(png_state* st,
                     png_u32 type,
                     const png_u8* chunk_data,
                     png_u32 length);

/* ---- Prototipos de TODOS los handlers ---- */

/* Core */

/* Core */
int png_handle_IHDR(png_state*, const png_u8*, png_u32);
int png_handle_PLTE(png_state*, const png_u8*, png_u32);
int png_handle_IDAT(png_state*, const png_u8*, png_u32);
int png_handle_IEND(png_state*, const png_u8*, png_u32); /* <-- ADD */
int png_idat_append(png_state*, const png_u8*, png_u32);


/* Color & transparencia */
int png_handle_tRNS(png_state*, const png_u8*, png_u32);
int png_handle_gAMA(png_state*, const png_u8*, png_u32);
int png_handle_cHRM(png_state*, const png_u8*, png_u32);
int png_handle_cICP(png_state*, const png_u8*, png_u32);
int png_handle_mDCV(png_state*, const png_u8*, png_u32);
int png_handle_cLLI(png_state*, const png_u8*, png_u32);
int png_handle_sRGB(png_state*, const png_u8*, png_u32);

/* Metadata física */
int png_handle_pHYs(png_state*, const png_u8*, png_u32);

/* Fondo, tiempo, bits significativos */
int png_handle_bKGD(png_state*, const png_u8*, png_u32);
int png_handle_tIME(png_state*, const png_u8*, png_u32);
int png_handle_sBIT(png_state*, const png_u8*, png_u32);

/* ---- CHUNKS FALTANTES AÑADIDOS ---- */

/* Texto */
int png_handle_tEXt(png_state*, const png_u8*, png_u32);
int png_handle_zTXt(png_state*, const png_u8*, png_u32);
int png_handle_iTXt(png_state*, const png_u8*, png_u32);

/* Color avanzado / ICC */
int png_handle_iCCP(png_state*, const png_u8*, png_u32);
int png_handle_eXIf(png_state*, const png_u8*, png_u32);

/* Paleta sugerida */
int png_handle_sPLT(png_state*, const png_u8*, png_u32);

/* Histograma */
int png_handle_hIST(png_state*, const png_u8*, png_u32);

/* Extensiones físicas */
int png_handle_oFFs(png_state*, const png_u8*, png_u32);
int png_handle_sCAL(png_state*, const png_u8*, png_u32);
int png_handle_pCAL(png_state*, const png_u8*, png_u32);

/* Otros especiales */
int png_handle_sTER(png_state*, const png_u8*, png_u32);
int png_handle_gIFg(png_state*, const png_u8*, png_u32);
int png_handle_gIFx(png_state*, const png_u8*, png_u32);
int png_handle_gIFt(png_state*, const png_u8*, png_u32);
int png_handle_dSIG(png_state*, const png_u8*, png_u32);
int png_handle_fRAc(png_state*, const png_u8*, png_u32);

#endif /* PNG_CHUNKS_H */
