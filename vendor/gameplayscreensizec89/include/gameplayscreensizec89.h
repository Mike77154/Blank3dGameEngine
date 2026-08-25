#ifndef GAMEPLAYSCREENSIZEC89_H
#define GAMEPLAYSCREENSIZEC89_H

/* Logical gameplay/room/camera drawing space. No renderer or OS ownership. */
#ifdef __cplusplus
extern "C" {
#endif

#define GPSS89_MIN_SIZE 1
#define GPSS89_MAX_SIZE 1048576

typedef struct gpss89_config {
    int camera_draw_width;
    int camera_draw_height;
    int scene_screen_width;
    int scene_screen_height;
    int camera_x;
    int camera_y;
    int clamp_camera_to_scene;
    unsigned int revision;
} gpss89_config;

typedef struct gpss89_provider {
    void *user;
    int (*apply_gameplay_screen)(void *user, const gpss89_config *config);
} gpss89_provider;

typedef struct gpss89_state {
    gpss89_config config;
    gpss89_provider provider;
    int provider_bound;
    char status[96];
} gpss89_state;

void gpss89_defaults(gpss89_state *state);
void gpss89_set_provider(gpss89_state *state, const gpss89_provider *provider);
int gpss89_set_camera_draw_size(gpss89_state *state, int width, int height);
int gpss89_set_scene_screen_size(gpss89_state *state, int width, int height);
int gpss89_set_camera_position(gpss89_state *state, int x, int y);
int gpss89_set_clamp(gpss89_state *state, int enabled);
void gpss89_clamp_camera(gpss89_state *state);
int gpss89_camera_fits_scene(const gpss89_state *state);
int gpss89_apply(gpss89_state *state);
const char *gpss89_status(const gpss89_state *state);

#ifdef __cplusplus
}
#endif
#endif
