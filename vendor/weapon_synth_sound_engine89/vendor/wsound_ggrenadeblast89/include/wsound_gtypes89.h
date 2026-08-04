#ifndef WSOUND_GTYPES89_H
#define WSOUND_GTYPES89_H

/*
 * C89 fixed-width assumptions.
 * Required target model:
 *   signed short  = at least 16 bits
 *   signed long   = at least 32 bits
 *   unsigned long = at least 32 bits
 */

typedef signed short ws_gs16;
typedef unsigned short ws_gu16;
typedef signed long ws_gs32;
typedef unsigned long ws_gu32;

#define WS_GQ15_ONE 32767
#define WS_GQ12_ONE 4096
#define WS_GU32_MASK 0xFFFFFFFFUL

ws_gs16 ws_gclip16(ws_gs32 x);
ws_gs16 ws_gsoftlimit16(ws_gs32 x);
ws_gs32 ws_gmul_q15(ws_gs32 a, ws_gs32 b);
ws_gs32 ws_gmul_q12(ws_gs32 a, ws_gs32 b);

#endif
