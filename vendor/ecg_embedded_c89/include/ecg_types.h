#ifndef ECG_TYPES_H
#define ECG_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#define ECG_VERSION_MAJOR 1u
#define ECG_VERSION_MINOR 2u
#define ECG_VERSION_PATCH 0u

#define ECG_SIGNAL_COLS 80u
#define ECG_PROFILE_COUNT 5u
#define ECG_VISIBLE_COLS_DEFAULT 32u
#define ECG_WAVEFORM_Y_UNITS_DEFAULT 30u

#define ECG_PROFILE_FINE 0u
#define ECG_PROFILE_CAUTION 1u
#define ECG_PROFILE_ORANGE 2u
#define ECG_PROFILE_DANGER 3u
#define ECG_PROFILE_POISON 4u

#define ECG_RENDER_DRAW_GRID   0x0001ul
#define ECG_RENDER_DRAW_AXIS   0x0002ul
#define ECG_RENDER_DRAW_FRAME  0x0004ul
#define ECG_RENDER_DRAW_SIGNAL 0x0008ul
#define ECG_RENDER_DRAW_WINDOW 0x0010ul
#define ECG_RENDER_DRAW_ALL \
    (ECG_RENDER_DRAW_GRID | ECG_RENDER_DRAW_AXIS | \
     ECG_RENDER_DRAW_FRAME | ECG_RENDER_DRAW_SIGNAL | \
     ECG_RENDER_DRAW_WINDOW)

#define ECG_SIGNAL_STYLE_CONTINUOUS 0u
#define ECG_SIGNAL_STYLE_DOTTED     1u
#define ECG_SIGNAL_STYLE_BARS       2u

typedef unsigned char ECG_U8;
typedef signed char ECG_I8;
typedef unsigned long ECG_U32;
typedef signed long ECG_FixedQ8;

typedef enum ECG_StatusTag {
    ECG_STATUS_OK = 0,
    ECG_STATUS_NULL = 1,
    ECG_STATUS_BAD_ARGUMENT = 2,
    ECG_STATUS_IO = 3,
    ECG_STATUS_CAPACITY = 4,
    ECG_STATUS_NOT_FOUND = 5
} ECG_Status;

typedef struct ECG_ColorTag {
    ECG_U8 r;
    ECG_U8 g;
    ECG_U8 b;
} ECG_Color;

typedef struct ECG_LineTag {
    ECG_I8 y;
    ECG_I8 h;
} ECG_Line;

typedef struct ECG_ProfileTag {
    const char *name;
    ECG_Color color;
    ECG_Color gradient;
    ECG_I8 samples[ECG_SIGNAL_COLS];
    ECG_Line lines[ECG_SIGNAL_COLS];
} ECG_Profile;

typedef struct ECG_SurfaceTag {
    ECG_Color *pixels;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
} ECG_Surface;

typedef struct ECG_RenderConfigTag {
    unsigned long draw_flags;
    ECG_FixedQ8 x_step_q8;
    ECG_FixedQ8 y_step_q8;
    unsigned int bar_width_px;
    unsigned int grid_x_units;
    unsigned int grid_y_units;
    unsigned int axis_y_units;
    unsigned int waveform_y_units;
    ECG_Color grid_color;
    ECG_Color axis_color;
    ECG_Color frame_color;
    ECG_Color window_color;

    unsigned int signal_style;
    unsigned int dot_on_px;
    unsigned int dot_off_px;
    unsigned int glow_enabled;
    unsigned int glow_radius_px;
    unsigned int glow_intensity;
    ECG_Color glow_color;
    unsigned int use_profile_glow_color;
} ECG_RenderConfig;

#ifdef __cplusplus
}
#endif

#endif
