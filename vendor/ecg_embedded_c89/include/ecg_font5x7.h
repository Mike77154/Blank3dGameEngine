#ifndef ECG_FONT5X7_H
#define ECG_FONT5X7_H

#include "ecg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ECG_U8 ecg_font5x7_row(char ch, unsigned int row);

ECG_Status ecg_draw_char5x7(ECG_Surface *surface, int x, int y,
                            char ch, ECG_Color color,
                            unsigned int scale);
ECG_Status ecg_draw_text5x7(ECG_Surface *surface, int x, int y,
                            const char *text, ECG_Color color,
                            unsigned int scale);
ECG_Status ecg_draw_char5x7_scaled_q8(ECG_Surface *surface, int x, int y,
                                      char ch, ECG_Color color,
                                      ECG_FixedQ8 scale_x_q8,
                                      ECG_FixedQ8 scale_y_q8);
ECG_Status ecg_draw_text5x7_scaled_q8(ECG_Surface *surface, int x, int y,
                                      const char *text, ECG_Color color,
                                      ECG_FixedQ8 scale_x_q8,
                                      ECG_FixedQ8 scale_y_q8);

#ifdef __cplusplus
}
#endif

#endif
