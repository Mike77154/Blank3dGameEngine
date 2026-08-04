#include "ecg_font5x7.h"
#include "ecg_surface.h"
#include "ecg_fixed.h"

static const ECG_U8 *ecg_glyph5x7(char ch)
{
    static const ECG_U8 blank[7] = {0,0,0,0,0,0,0};
    static const ECG_U8 A[7] = {14,17,17,31,17,17,17};
    static const ECG_U8 B[7] = {30,17,17,30,17,17,30};
    static const ECG_U8 C[7] = {14,17,16,16,16,17,14};
    static const ECG_U8 D[7] = {30,17,17,17,17,17,30};
    static const ECG_U8 E[7] = {31,16,16,30,16,16,31};
    static const ECG_U8 F[7] = {31,16,16,30,16,16,16};
    static const ECG_U8 G[7] = {14,17,16,23,17,17,14};
    static const ECG_U8 H[7] = {17,17,17,31,17,17,17};
    static const ECG_U8 I[7] = {14,4,4,4,4,4,14};
    static const ECG_U8 J[7] = {1,1,1,1,17,17,14};
    static const ECG_U8 K[7] = {17,18,20,24,20,18,17};
    static const ECG_U8 L[7] = {16,16,16,16,16,16,31};
    static const ECG_U8 M[7] = {17,27,21,21,17,17,17};
    static const ECG_U8 N[7] = {17,25,21,19,17,17,17};
    static const ECG_U8 O[7] = {14,17,17,17,17,17,14};
    static const ECG_U8 P[7] = {30,17,17,30,16,16,16};
    static const ECG_U8 Q[7] = {14,17,17,17,21,18,13};
    static const ECG_U8 R[7] = {30,17,17,30,20,18,17};
    static const ECG_U8 S[7] = {15,16,16,14,1,1,30};
    static const ECG_U8 T[7] = {31,4,4,4,4,4,4};
    static const ECG_U8 U[7] = {17,17,17,17,17,17,14};
    static const ECG_U8 V[7] = {17,17,17,17,17,10,4};
    static const ECG_U8 W[7] = {17,17,17,21,21,21,10};
    static const ECG_U8 X[7] = {17,17,10,4,10,17,17};
    static const ECG_U8 Y[7] = {17,17,10,4,4,4,4};
    static const ECG_U8 Z[7] = {31,1,2,4,8,16,31};
    static const ECG_U8 n0[7] = {14,17,19,21,25,17,14};
    static const ECG_U8 n1[7] = {4,12,4,4,4,4,14};
    static const ECG_U8 n2[7] = {14,17,1,2,4,8,31};
    static const ECG_U8 n3[7] = {30,1,1,14,1,1,30};
    static const ECG_U8 n4[7] = {2,6,10,18,31,2,2};
    static const ECG_U8 n5[7] = {31,16,16,30,1,1,30};
    static const ECG_U8 n6[7] = {14,16,16,30,17,17,14};
    static const ECG_U8 n7[7] = {31,1,2,4,8,8,8};
    static const ECG_U8 n8[7] = {14,17,17,14,17,17,14};
    static const ECG_U8 n9[7] = {14,17,17,15,1,1,14};
    static const ECG_U8 colon[7] = {0,4,4,0,4,4,0};
    static const ECG_U8 dash[7] = {0,0,0,31,0,0,0};
    static const ECG_U8 slash[7] = {1,2,2,4,8,8,16};
    static const ECG_U8 dot[7] = {0,0,0,0,0,4,4};
    static const ECG_U8 percent[7] = {17,2,4,4,8,16,17};
    static const ECG_U8 plus[7] = {0,4,4,31,4,4,0};

    switch (ch) {
        case 'A': return A; case 'B': return B; case 'C': return C;
        case 'D': return D; case 'E': return E; case 'F': return F;
        case 'G': return G; case 'H': return H; case 'I': return I;
        case 'J': return J; case 'K': return K; case 'L': return L;
        case 'M': return M; case 'N': return N; case 'O': return O;
        case 'P': return P; case 'Q': return Q; case 'R': return R;
        case 'S': return S; case 'T': return T; case 'U': return U;
        case 'V': return V; case 'W': return W; case 'X': return X;
        case 'Y': return Y; case 'Z': return Z;
        case '0': return n0; case '1': return n1; case '2': return n2;
        case '3': return n3; case '4': return n4; case '5': return n5;
        case '6': return n6; case '7': return n7; case '8': return n8;
        case '9': return n9; case ':': return colon; case '-': return dash;
        case '/': return slash; case '.': return dot; case '%': return percent;
        case '+': return plus;
        default: return blank;
    }
}

ECG_U8 ecg_font5x7_row(char ch, unsigned int row)
{
    const ECG_U8 *glyph;
    if (row >= 7u) return 0u;
    glyph = ecg_glyph5x7(ch);
    return glyph[row];
}

ECG_Status ecg_draw_char5x7_scaled_q8(ECG_Surface *surface, int x, int y,
                                      char ch, ECG_Color color,
                                      ECG_FixedQ8 scale_x_q8,
                                      ECG_FixedQ8 scale_y_q8)
{
    const ECG_U8 *glyph;
    unsigned int row;
    unsigned int col;
    int left;
    int right;
    int top;
    int bottom;
    unsigned int width;
    unsigned int height;

    if (!ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }
    if (scale_x_q8 <= (ECG_FixedQ8)0 ||
        scale_y_q8 <= (ECG_FixedQ8)0) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    glyph = ecg_glyph5x7(ch);
    for (row = 0u; row < 7u; row++) {
        top = y + ecg_fixed_mul_uint_to_int_floor(row, scale_y_q8);
        bottom = y + ecg_fixed_mul_uint_to_int_ceil(row + 1u,
                                                    scale_y_q8);
        height = bottom > top ? (unsigned int)(bottom - top) : 1u;

        for (col = 0u; col < 5u; col++) {
            if (((glyph[row] >> (4u - col)) & 1u) != 0u) {
                left = x + ecg_fixed_mul_uint_to_int_floor(col,
                                                           scale_x_q8);
                right = x + ecg_fixed_mul_uint_to_int_ceil(col + 1u,
                                                            scale_x_q8);
                width = right > left ? (unsigned int)(right - left) : 1u;
                (void)ecg_fill_rect(surface,
                                    left,
                                    top,
                                    width,
                                    height,
                                    color);
            }
        }
    }

    return ECG_STATUS_OK;
}

ECG_Status ecg_draw_text5x7_scaled_q8(ECG_Surface *surface, int x, int y,
                                      const char *text, ECG_Color color,
                                      ECG_FixedQ8 scale_x_q8,
                                      ECG_FixedQ8 scale_y_q8)
{
    unsigned int i;
    int advance;

    if (!ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }
    if (text == (const char *)0) {
        return ECG_STATUS_NULL;
    }
    if (scale_x_q8 <= (ECG_FixedQ8)0 ||
        scale_y_q8 <= (ECG_FixedQ8)0) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    i = 0u;
    while (text[i] != '\0') {
        advance = ecg_fixed_mul_uint_to_int_floor(i * 6u, scale_x_q8);
        if (text[i] != ' ') {
            (void)ecg_draw_char5x7_scaled_q8(surface,
                                             x + advance,
                                             y,
                                             text[i],
                                             color,
                                             scale_x_q8,
                                             scale_y_q8);
        }
        i++;
    }

    return ECG_STATUS_OK;
}

ECG_Status ecg_draw_char5x7(ECG_Surface *surface, int x, int y,
                            char ch, ECG_Color color,
                            unsigned int scale)
{
    if (scale == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    return ecg_draw_char5x7_scaled_q8(surface,
                                      x,
                                      y,
                                      ch,
                                      color,
                                      ecg_fixed_from_int((int)scale),
                                      ecg_fixed_from_int((int)scale));
}

ECG_Status ecg_draw_text5x7(ECG_Surface *surface, int x, int y,
                            const char *text, ECG_Color color,
                            unsigned int scale)
{
    if (scale == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    return ecg_draw_text5x7_scaled_q8(surface,
                                      x,
                                      y,
                                      text,
                                      color,
                                      ecg_fixed_from_int((int)scale),
                                      ecg_fixed_from_int((int)scale));
}
