#ifndef SKYBOXRECIPE89_H
#define SKYBOXRECIPE89_H

/*
 * SkyboxRecipe89 - renderer/filesystem agnostic INI sky recipe composer.
 * C89, caller-owned storage, no heap, fixed-point configuration.
 *
 * A recipe may include other recipes recursively. The host provides text IO.
 * Includes are processed first; fields in the including file override them.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define SBR89_VERSION_MAJOR 0
#define SBR89_VERSION_MINOR 1
#define SBR89_VERSION_PATCH 0

#define SBR89_FACE_COUNT 6
#define SBR89_FACE_PX 0
#define SBR89_FACE_NX 1
#define SBR89_FACE_PY 2
#define SBR89_FACE_NY 3
#define SBR89_FACE_PZ 4
#define SBR89_FACE_NZ 5

#define SBR89_NAME_CAP 64
#define SBR89_PATH_CAP 256
#define SBR89_REQUEST_CAP 192
#define SBR89_TEXT_CAP 8192
#define SBR89_MAX_INCLUDE_DEPTH 8

#define SBR89_FX_SHIFT 16
#define SBR89_FX_ONE 65536L

#define SBR89_SOURCE_PROCEDURAL 0
#define SBR89_SOURCE_FACES6 1
#define SBR89_SOURCE_ATLAS 2
#define SBR89_SOURCE_SINGLE_SCREEN 3
#define SBR89_SOURCE_SINGLE_DOME 4
#define SBR89_SOURCE_FAMILY 5

#define SBR89_LAYOUT_NONE 0
#define SBR89_LAYOUT_STRIP_6X1 1
#define SBR89_LAYOUT_STRIP_1X6 2
#define SBR89_LAYOUT_CROSS_4X3 3
#define SBR89_LAYOUT_CROSS_3X4 4
#define SBR89_LAYOUT_GRID_3X2 5
#define SBR89_LAYOUT_GRID_2X3 6
#define SBR89_LAYOUT_CUSTOM 7

#define SBR89_CONVENTION_AXIS 0
#define SBR89_CONVENTION_SOURCE 1
#define SBR89_CONVENTION_WORD 2

#define SBR89_BLEND_OFF 0
#define SBR89_BLEND_ALPHA 1
#define SBR89_BLEND_ADD 2

typedef struct SBR89ColorTag {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} SBR89Color;

typedef struct SBR89RectTag {
    long u0_q16;
    long v0_q16;
    long u1_q16;
    long v1_q16;
} SBR89Rect;

typedef struct SBR89RecipeTag {
    int enabled;
    int screen_enabled;
    int cube_enabled;
    int dome_enabled;
    long radius_q16;

    int source_type;
    int layout;
    int convention;
    int uv_inset_pixels;

    char shared_image[SBR89_REQUEST_CAP];
    char screen_image[SBR89_REQUEST_CAP];
    char dome_image[SBR89_REQUEST_CAP];
    char face_image[SBR89_FACE_COUNT][SBR89_REQUEST_CAP];
    SBR89Rect face_uv[SBR89_FACE_COUNT];

    char family_base[SBR89_REQUEST_CAP];
    char family_extension[24];

    int flip_u_mask;
    int flip_v_mask;
    int swap_uv_mask;

    int screen_depth_func;
    SBR89Color screen_top;
    SBR89Color screen_bottom;

    int dome_segments;
    int dome_rings;
    int dome_blend_mode;
    SBR89Color dome_top;
    SBR89Color dome_horizon;

    char loaded_recipe[SBR89_PATH_CAP];
    char selected_name[SBR89_NAME_CAP];
} SBR89Recipe;

typedef int (*SBR89LoadTextFn)(void *user,
                               const char *path,
                               char *out_text,
                               unsigned int capacity,
                               unsigned int *out_size);

typedef struct SBR89IOProviderTag {
    void *user;
    SBR89LoadTextFn load_text;
} SBR89IOProvider;

typedef struct SBR89WorkspaceTag {
    char text[SBR89_MAX_INCLUDE_DEPTH][SBR89_TEXT_CAP];
    char path_stack[SBR89_MAX_INCLUDE_DEPTH][SBR89_PATH_CAP];
    char last_error[160];
    int last_error_line;
} SBR89Workspace;

void sbr89_recipe_defaults(SBR89Recipe *recipe);
void sbr89_workspace_init(SBR89Workspace *workspace);
const char *sbr89_last_error(const SBR89Workspace *workspace);
int sbr89_last_error_line(const SBR89Workspace *workspace);

int sbr89_load_recipe(SBR89Recipe *out_recipe,
                      SBR89Workspace *workspace,
                      const SBR89IOProvider *io,
                      const char *recipe_path);

int sbr89_catalog_resolve(SBR89Workspace *workspace,
                          const SBR89IOProvider *io,
                          const char *catalog_path,
                          const char *recipe_name,
                          char *out_recipe_path,
                          unsigned int out_capacity);

int sbr89_load_named_recipe(SBR89Recipe *out_recipe,
                            SBR89Workspace *workspace,
                            const SBR89IOProvider *io,
                            const char *catalog_path,
                            const char *recipe_name);

int sbr89_source_type_from_text(const char *text);
int sbr89_layout_from_text(const char *text);
int sbr89_convention_from_text(const char *text);
int sbr89_apply_layout(SBR89Recipe *recipe);
int sbr89_build_family_faces(SBR89Recipe *recipe);

#ifdef __cplusplus
}
#endif

#endif
