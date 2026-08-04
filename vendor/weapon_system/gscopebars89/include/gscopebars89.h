/*
 * gscopebars89.h
 * Generic bar/channel bridge for scope HUDs.
 * C89, fixed-point only, no dynamic allocation.
 */
#ifndef GSCOPEBARS89_H
#define GSCOPEBARS89_H

#include "gscopepaint89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSCOPEBARS89_API
#define GSCOPEBARS89_API
#endif

#define GSB89_CHANNEL_HEALTH          1
#define GSB89_CHANNEL_STAMINA         2
#define GSB89_CHANNEL_BREATH          3
#define GSB89_CHANNEL_STABILITY       4
#define GSB89_CHANNEL_SWAY            5
#define GSB89_CHANNEL_ZOOM            6
#define GSB89_CHANNEL_AMMO            7
#define GSB89_CHANNEL_RELOAD          8
#define GSB89_CHANNEL_LOCK_PROGRESS   9
#define GSB89_CHANNEL_TARGET_HEALTH   10
#define GSB89_CHANNEL_RANGE_CONFIDENCE 11
#define GSB89_CHANNEL_HEAT            12
#define GSB89_CHANNEL_BATTERY         13
#define GSB89_CHANNEL_CUSTOM0         32

#define GSB89_BAR_LINEAR      0
#define GSB89_BAR_SEGMENTED   1
#define GSB89_BAR_ARC         2
#define GSB89_BAR_TICKS       3
#define GSB89_BAR_MARKER      4

#define GSB89_DIR_LEFT_TO_RIGHT  0
#define GSB89_DIR_RIGHT_TO_LEFT  1
#define GSB89_DIR_TOP_TO_BOTTOM  2
#define GSB89_DIR_BOTTOM_TO_TOP  3
#define GSB89_DIR_CLOCKWISE      4
#define GSB89_DIR_COUNTERCLOCKWISE 5

#define GSB89_VISIBLE             1
#define GSB89_SHOW_BACKGROUND     2
#define GSB89_SHOW_OUTLINE        4
#define GSB89_HIDE_WHEN_FULL      8
#define GSB89_HIDE_WHEN_EMPTY     16
#define GSB89_INVERT_VALUE        32
#define GSB89_SHOW_GLYPH          64

typedef struct gsb89_channel {
    short channel_id;
    long value;
    long minimum;
    long maximum;
    short visible;
} gsb89_channel;

typedef int (*gsb89_channel_cb)(void *user,
                                short channel_id,
                                gsb89_channel *out_channel);

typedef struct gsb89_bar {
    short bar_id;
    short channel_id;
    short kind;
    short direction;
    short flags;
    short layer;
    short part_id;
    short segment_count;
    short glyph_id;

    gsp89_fx x0;
    gsp89_fx y0;
    gsp89_fx x1;
    gsp89_fx y1;
    gsp89_fx thickness;
    gsp89_fx radius_x;
    gsp89_fx radius_y;
    short start_deg_x100;
    short end_deg_x100;

    gsp89_color foreground;
    gsp89_color background;
    gsp89_color outline;
    short thickness_px;
    short outline_px;
    short blend_mode;
} gsb89_bar;

typedef struct gsb89_layout {
    const gsb89_bar *bars;
    short bar_count;
} gsb89_layout;

GSCOPEBARS89_API gsb89_bar gsb89_bar_make(short bar_id,
                                           short channel_id,
                                           short kind,
                                           short direction,
                                           gsp89_fx x0,
                                           gsp89_fx y0,
                                           gsp89_fx x1,
                                           gsp89_fx y1);
GSCOPEBARS89_API gsp89_fx gsb89_channel_ratio(const gsb89_channel *channel);
GSCOPEBARS89_API int gsb89_find_channel(const gsb89_channel *channels,
                                        short channel_count,
                                        short channel_id,
                                        gsb89_channel *out_channel);
GSCOPEBARS89_API void gsb89_emit_bar(gsp89_painter *painter,
                                     const gsb89_bar *bar,
                                     const gsb89_channel *channel,
                                     short global_alpha);
GSCOPEBARS89_API void gsb89_emit_layout_from_array(gsp89_painter *painter,
                                                   const gsb89_layout *layout,
                                                   const gsb89_channel *channels,
                                                   short channel_count,
                                                   short global_alpha);
GSCOPEBARS89_API void gsb89_emit_layout_from_callback(gsp89_painter *painter,
                                                      const gsb89_layout *layout,
                                                      gsb89_channel_cb channel_cb,
                                                      void *user,
                                                      short global_alpha);

#ifdef __cplusplus
}
#endif

#endif
