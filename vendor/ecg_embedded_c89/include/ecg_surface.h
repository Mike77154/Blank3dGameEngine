#ifndef ECG_SURFACE_H
#define ECG_SURFACE_H

#include "ecg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ECG_Color ecg_color_make(ECG_U8 r, ECG_U8 g, ECG_U8 b);
ECG_Color ecg_color_fade_step(ECG_Color base, ECG_Color gradient, unsigned int step);
ECG_Color ecg_color_scale(ECG_Color base, unsigned int numerator, unsigned int denominator);

int ecg_surface_is_valid(const ECG_Surface *surface);
ECG_Status ecg_surface_clear(ECG_Surface *surface, ECG_Color color);
void ecg_put_pixel(ECG_Surface *surface, int x, int y, ECG_Color color);
void ecg_put_pixel_additive(ECG_Surface *surface, int x, int y,
                            ECG_Color color);
void ecg_put_pixel_lighten(ECG_Surface *surface, int x, int y,
                           ECG_Color color);
ECG_Status ecg_fill_rect(ECG_Surface *surface, int x, int y,
                         unsigned int width, unsigned int height,
                         ECG_Color color);
ECG_Status ecg_draw_hline(ECG_Surface *surface, int x0, int x1, int y,
                          ECG_Color color);
ECG_Status ecg_draw_vline(ECG_Surface *surface, int x, int y0, int y1,
                          ECG_Color color);
ECG_Status ecg_draw_rect_outline(ECG_Surface *surface, int x, int y,
                                 unsigned int width, unsigned int height,
                                 ECG_Color color);

#ifdef __cplusplus
}
#endif

#endif
