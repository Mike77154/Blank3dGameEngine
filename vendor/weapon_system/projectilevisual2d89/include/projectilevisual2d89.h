#ifndef PROJECTILEVISUAL2D89_H
#define PROJECTILEVISUAL2D89_H

#ifdef __cplusplus
extern "C" {
#endif

#define PV2D89_VERSION_MAJOR 0
#define PV2D89_VERSION_MINOR 1
#define PV2D89_VERSION_PATCH 0

#define PV2D89_Q16_ONE 65536L
#define PV2D89_MAX_PASSES 3

#define PV2D89_BILLBOARD_NONE 0
#define PV2D89_BILLBOARD_VIEW 1

#define PV2D89_BLEND_ALPHA 0
#define PV2D89_BLEND_ADDITIVE 1

#define PV2D89_PASS_GLOW 0
#define PV2D89_PASS_MAIN 1
#define PV2D89_PASS_CORE 2

#define PV2D89_FRAME_SOURCE_RECT 0x0001U
#define PV2D89_FRAME_FLIP_X      0x0002U
#define PV2D89_FRAME_FLIP_Y      0x0004U

#define PV2D89_OK 1
#define PV2D89_NO_VISUAL 0
#define PV2D89_ERR_ARGUMENT -1
#define PV2D89_ERR_PROVIDER -2
#define PV2D89_ERR_FRAME -3

typedef struct PV2D89Vec3Tag {
    long x;
    long y;
    long z;
} PV2D89Vec3;

typedef struct PV2D89ColorTag {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} PV2D89Color;

typedef struct PV2D89FrameTag {
    int valid;
    int image_handle;
    int source_x;
    int source_y;
    int source_w;
    int source_h;
    long scale_x_q16;
    long scale_y_q16;
    unsigned int frame_index;
    unsigned int flags;
} PV2D89Frame;

typedef struct PV2D89FrameProviderTag {
    void *user;
    int (*sample_frame)(void *user,
                        int weapon_id,
                        unsigned int age_ms,
                        int projectile_slot,
                        PV2D89Frame *out_frame);
} PV2D89FrameProvider;

typedef struct PV2D89RecipeTag {
    int enabled;
    int weapon_id;
    int billboard_mode;
    int replace_mesh;
    int blend_mode;

    long width_scale_q16;
    long height_scale_q16;
    long pulse_min_q16;
    long pulse_max_q16;
    unsigned int phase_step;
    int phase_flip_x;

    int age_fade;
    PV2D89Color main_start;
    PV2D89Color main_mid;
    PV2D89Color main_end;

    int glow_enabled;
    long glow_scale_q16;
    PV2D89Color glow_color;

    int core_enabled;
    long core_scale_q16;
    PV2D89Color core_color;

    int light_enabled;
    long light_intensity_q16;
    long light_radius_q16;
    PV2D89Color light_color;
} PV2D89Recipe;

typedef struct PV2D89ProjectileInputTag {
    PV2D89Vec3 position_q16;
    PV2D89Vec3 camera_right_q16;
    PV2D89Vec3 camera_up_q16;
    long mesh_scale_q16;
    unsigned int age_ms;
    unsigned int life_ms;
    int projectile_slot;
} PV2D89ProjectileInput;

typedef struct PV2D89RenderPassTag {
    int enabled;
    int layer;
    int image_handle;
    int blend_mode;
    int source_enabled;
    int source_x;
    int source_y;
    int source_w;
    int source_h;
    int flip_x;
    int flip_y;
    PV2D89Color tint;
    PV2D89Vec3 center_q16;
    PV2D89Vec3 right_q16;
    PV2D89Vec3 up_q16;
    long width_q16;
    long height_q16;
    PV2D89Vec3 corners_q16[4]; /* TL, TR, BR, BL */
} PV2D89RenderPass;

typedef struct PV2D89LightSampleTag {
    int enabled;
    int group_id;
    PV2D89Vec3 position_q16;
    long intensity_q16;
    long radius_q16;
    PV2D89Color color;
} PV2D89LightSample;

typedef struct PV2D89SampleTag {
    int valid;
    int replace_mesh;
    unsigned int frame_index;
    int pass_count;
    PV2D89RenderPass passes[PV2D89_MAX_PASSES];
    PV2D89LightSample light;
} PV2D89Sample;

typedef struct PV2D89LightAccumulatorTag {
    int used;
    int group_id;
    int count;
    PV2D89Vec3 center_q16;
    long base_intensity_q16;
    long base_radius_q16;
    PV2D89Color color;
} PV2D89LightAccumulator;

void pv2d89_recipe_init(PV2D89Recipe *recipe);
void pv2d89_sample_clear(PV2D89Sample *sample);
int pv2d89_wants_billboard(const PV2D89Recipe *recipe);
int pv2d89_build(const PV2D89Recipe *recipe,
                  const PV2D89ProjectileInput *input,
                  const PV2D89FrameProvider *provider,
                  PV2D89Sample *out_sample);

void pv2d89_light_accumulator_init(PV2D89LightAccumulator *accum);
int pv2d89_light_accumulator_add(PV2D89LightAccumulator *accum,
                                 const PV2D89LightSample *sample);
int pv2d89_light_accumulator_finish(const PV2D89LightAccumulator *accum,
                                    unsigned long frame_stamp,
                                    PV2D89LightSample *out_light);

#ifdef __cplusplus
}
#endif

#endif
