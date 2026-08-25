#ifndef GENERALVIDEOCONFIGC89_H
#define GENERALVIDEOCONFIGC89_H

/* Device/renderer-oriented video presentation configuration. */
#ifdef __cplusplus
extern "C" {
#endif

#define GVC89_MIN_RESOLUTION 32
#define GVC89_MAX_RESOLUTION 8192

#define GVC89_SCALE_NATIVE 1
#define GVC89_SCALE_FIT 2
#define GVC89_SCALE_STRETCH 3
#define GVC89_SCALE_INTEGER 4
#define GVC89_SCALE_OVERSCAN 5
#define GVC89_SCALE_FIXED 6

#define GVC89_FILTER_NEAREST 1
#define GVC89_FILTER_LINEAR 2

typedef struct gvc89_rect {
    int x;
    int y;
    int width;
    int height;
} gvc89_rect;

typedef struct gvc89_config {
    int resolution_width;
    int resolution_height;
    int scale_mode;
    int fixed_scale;
    int keep_aspect;
    int center_output;
    int filter_mode;
    int vsync;
    unsigned int revision;
} gvc89_config;

typedef struct gvc89_provider {
    void *user;
    int (*apply_video)(void *user, const gvc89_config *config,
                       const gvc89_rect *presentation,
                       int output_width, int output_height);
} gvc89_provider;

typedef struct gvc89_state {
    gvc89_config config;
    gvc89_rect presentation;
    int output_width;
    int output_height;
    gvc89_provider provider;
    int provider_bound;
    char status[96];
} gvc89_state;

void gvc89_defaults(gvc89_state *state);
void gvc89_set_provider(gvc89_state *state, const gvc89_provider *provider);
int gvc89_load_text(gvc89_state *state, const char *text);
int gvc89_set_resolution(gvc89_state *state, int width, int height);
int gvc89_set_output_size(gvc89_state *state, int width, int height);
int gvc89_set_scale_mode(gvc89_state *state, int mode, int fixed_scale);
int gvc89_set_filter(gvc89_state *state, int mode);
int gvc89_set_vsync(gvc89_state *state, int enabled);
int gvc89_set_keep_aspect(gvc89_state *state, int enabled);
int gvc89_set_center_output(gvc89_state *state, int enabled);
int gvc89_compute_presentation(gvc89_state *state);
int gvc89_apply(gvc89_state *state);
int gvc89_scale_from_name(const char *name, int *out_mode, int *out_fixed_scale);
const char *gvc89_scale_name(int mode);
int gvc89_filter_from_name(const char *name, int *out_mode);
const char *gvc89_status(const gvc89_state *state);

#ifdef __cplusplus
}
#endif
#endif
