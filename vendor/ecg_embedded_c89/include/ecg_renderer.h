#ifndef ECG_RENDERER_H
#define ECG_RENDERER_H

#include "ecg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void ecg_render_config_default(ECG_RenderConfig *config_out);

ECG_Status ecg_draw_grid(ECG_Surface *surface, int x, int y,
                         unsigned int column_count,
                         const ECG_RenderConfig *config);

ECG_Status ecg_draw_column_bar(ECG_Surface *surface, int x, int top_y,
                               ECG_Line line, ECG_Color color,
                               const ECG_RenderConfig *config);

ECG_Status ecg_draw_active_window(ECG_Surface *surface,
                                  const ECG_Profile *profile,
                                  int x0, int y0,
                                  unsigned int offset,
                                  unsigned int visible_cols,
                                  const ECG_RenderConfig *config);

ECG_Status ecg_draw_overview(ECG_Surface *surface,
                             const ECG_Profile *profile,
                             int x0, int y0,
                             unsigned int offset,
                             unsigned int visible_cols,
                             const ECG_RenderConfig *config);

#ifdef __cplusplus
}
#endif

#endif
