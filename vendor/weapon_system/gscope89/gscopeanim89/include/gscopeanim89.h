/*
 * gscopeanim89.h - INI-driven reticle/HUD animation sampler.
 * C89, integer/fixed-point only, caller-owned/static storage, no heap.
 */
#ifndef GSCOPEANIM89_H
#define GSCOPEANIM89_H

#include "gscopeini89.h"
#include "gscopevector89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSCOPEANIM89_API
#define GSCOPEANIM89_API
#endif

#define GSA89_ABI_VERSION 1u
#define GSA89_MAX_CLIPS 32
#define GSA89_MAX_CHANNELS 128
#define GSA89_MAX_NAME 32
#define GSA89_MAX_TRIGGER 32

#define GSA89_MODE_ONESHOT 0
#define GSA89_MODE_LOOP    1

#define GSA89_EASE_LINEAR      0
#define GSA89_EASE_IN_QUAD     1
#define GSA89_EASE_OUT_QUAD    2
#define GSA89_EASE_IN_OUT_QUAD 3
#define GSA89_EASE_STEP        4

#define GSA89_BLEND_REPLACE 0
#define GSA89_BLEND_ADD     1
#define GSA89_BLEND_MULTIPLY 2

#define GSA89_PROP_SCALE_X1000    0
#define GSA89_PROP_OFFSET_X_NORM  1
#define GSA89_PROP_OFFSET_Y_NORM  2
#define GSA89_PROP_ALPHA_X1000    3

#define GSA89_TARGET_ROOT (-1)

#define GSA89_OK 0
#define GSA89_ERR_RECIPE 1
#define GSA89_ERR_OVERFLOW 2
#define GSA89_ERR_PARSE 3

typedef struct gsa89_clip {
    char name[GSA89_MAX_NAME];
    char trigger[GSA89_MAX_TRIGGER];
    short mode;
    unsigned short duration_ms;
    unsigned short elapsed_ms;
    short active;
} gsa89_clip;

typedef struct gsa89_channel {
    char name[GSA89_MAX_NAME];
    short clip_id;
    short target_part; /* -1 = root/all, otherwise GSV89_PART_* */
    short property;
    long from_value;
    long to_value;
    unsigned short start_ms;
    unsigned short duration_ms;
    short ease;
    short blend;
} gsa89_channel;

typedef struct gsa89_pose_part {
    long scale_x1000;
    long offset_x_norm;
    long offset_y_norm;
    long alpha_x1000;
} gsa89_pose_part;

typedef struct gsa89_pose {
    gsa89_pose_part root;
    gsa89_pose_part part[GSV89_MAX_PARTS];
} gsa89_pose;

typedef struct gsa89_ctx {
    gsa89_clip clips[GSA89_MAX_CLIPS];
    gsa89_channel channels[GSA89_MAX_CHANNELS];
    short clip_count;
    short channel_count;
    short last_error;
} gsa89_ctx;

GSCOPEANIM89_API void gsa89_init(gsa89_ctx *ctx);
GSCOPEANIM89_API int gsa89_load(gsa89_ctx *ctx, const char *path);
GSCOPEANIM89_API int gsa89_load_doc(gsa89_ctx *ctx, const gri89_doc *doc);
GSCOPEANIM89_API void gsa89_reset(gsa89_ctx *ctx);
GSCOPEANIM89_API short gsa89_trigger(gsa89_ctx *ctx, const char *trigger_name);
GSCOPEANIM89_API void gsa89_tick(gsa89_ctx *ctx, unsigned short dt_ms);
GSCOPEANIM89_API void gsa89_sample(const gsa89_ctx *ctx, gsa89_pose *out_pose);
GSCOPEANIM89_API short gsa89_apply_shapes(const gsa89_pose *pose,
                                           const gsv89_shape *src,
                                           short shape_count,
                                           gsv89_shape *dst,
                                           short dst_capacity);
GSCOPEANIM89_API short gsa89_apply_alpha(const gsa89_pose *pose,
                                          short global_alpha,
                                          short part_id);
GSCOPEANIM89_API short gsa89_last_error(const gsa89_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif
