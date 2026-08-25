#ifndef GSKYBOX89_ASSETS_H
#define GSKYBOX89_ASSETS_H

/*
 * gskybox89_assets - tiny skybox asset convention helper.
 * C89, no heap, fixed capacity strings.
 *
 * This is deliberately a mapper, not a heavy texture decoder. It turns
 * common skybox file naming conventions into the six gskybox89 faces so a
 * renderer/authoring tool can bind the right texture per emitted face.
 */

#include "gskybox89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GSKYBOX89_ASSET_VERSION_MAJOR 1
#define GSKYBOX89_ASSET_VERSION_MINOR 0
#define GSKYBOX89_ASSET_VERSION_PATCH 0

#ifndef GSKYBOX89_ASSET_PATH_MAX
#define GSKYBOX89_ASSET_PATH_MAX 260
#endif

#ifndef GSKYBOX89_ASSET_KEY_MAX
#define GSKYBOX89_ASSET_KEY_MAX 96
#endif

#define GSKYBOX89_ASSET_CONV_UNKNOWN 0
#define GSKYBOX89_ASSET_CONV_SOURCE6 1  /* FT/BK/LF/RT/UP/DN, VMT/VTF/TGA/BMP */
#define GSKYBOX89_ASSET_CONV_AXIS6   2  /* px/nx/py/ny/pz/nz */
#define GSKYBOX89_ASSET_CONV_WORD6   3  /* right/left/up/down/front/back */

#define GSKYBOX89_ASSET_OK 0
#define GSKYBOX89_ASSET_ERR_NULL -1
#define GSKYBOX89_ASSET_ERR_NOT_FOUND -2
#define GSKYBOX89_ASSET_ERR_RANGE -3
#define GSKYBOX89_ASSET_ERR_IO -4

#define GSKYBOX89_ASSET_FLAG_ALLOW_VMT 1
#define GSKYBOX89_ASSET_FLAG_ALLOW_VTF 2
#define GSKYBOX89_ASSET_FLAG_ALLOW_IMAGES 4
#define GSKYBOX89_ASSET_FLAG_DEFAULT (GSKYBOX89_ASSET_FLAG_ALLOW_VMT | GSKYBOX89_ASSET_FLAG_ALLOW_VTF | GSKYBOX89_ASSET_FLAG_ALLOW_IMAGES)

typedef struct Gskybox89_AssetSet {
    char face_path[GSKYBOX89_CUBE_FACE_COUNT][GSKYBOX89_ASSET_PATH_MAX];
    char face_key[GSKYBOX89_CUBE_FACE_COUNT][GSKYBOX89_ASSET_KEY_MAX];
    int present[GSKYBOX89_CUBE_FACE_COUNT];
    int convention[GSKYBOX89_CUBE_FACE_COUNT];
    int present_count;
} Gskybox89_AssetSet;

void gskybox89_asset_set_clear(Gskybox89_AssetSet *set);
int gskybox89_asset_add_path(Gskybox89_AssetSet *set, const char *path, int flags);
int gskybox89_asset_face_from_name(const char *path_or_name, int *out_face, int *out_convention);
int gskybox89_asset_complete_mask(const Gskybox89_AssetSet *set);
void gskybox89_asset_apply_uv_preset(Gskybox89_Config *cfg, int convention);
const char *gskybox89_asset_convention_name(int convention);
const char *gskybox89_asset_source_suffix_for_face(int face);
const char *gskybox89_asset_axis_suffix_for_face(int face);

/* Tiny Source text helpers. They parse one file with fixed local buffers. */
int gskybox89_asset_vmf_extract_skyname(const char *vmf_path, char *out_skyname, int out_cap);
int gskybox89_asset_vmt_extract_basetexture(const char *vmt_path, char *out_basetexture, int out_cap);

#ifdef __cplusplus
}
#endif

#endif
