#ifndef BLANK3D_SKYBOX_RECIPE89_H
#define BLANK3D_SKYBOX_RECIPE89_H

#include "blank3d_skybox89.h"
#include "skyboxrecipe89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DSkyboxRecipe89Tag {
    SBR89Workspace workspace;
    SBR89Recipe active;
    SBR89IOProvider io;
    Blank3DSkybox89 *skybox;
    char catalog_path[SBR89_PATH_CAP];
    char active_name[SBR89_NAME_CAP];
    char active_ref[SBR89_PATH_CAP];
    int active_is_path;
    char status[192];
} Blank3DSkyboxRecipe89;

void blank3d_skybox_recipe89_init(Blank3DSkyboxRecipe89 *ctx,
                                  Blank3DSkybox89 *skybox);
void blank3d_skybox_recipe89_set_catalog(Blank3DSkyboxRecipe89 *ctx,
                                         const char *catalog_path);
int blank3d_skybox_recipe89_apply_named(Blank3DSkyboxRecipe89 *ctx,
                                        const char *recipe_name);
int blank3d_skybox_recipe89_apply_path(Blank3DSkyboxRecipe89 *ctx,
                                       const char *recipe_path);
int blank3d_skybox_recipe89_reload(Blank3DSkyboxRecipe89 *ctx);
void blank3d_skybox_recipe89_disable(Blank3DSkyboxRecipe89 *ctx);
const char *blank3d_skybox_recipe89_status(const Blank3DSkyboxRecipe89 *ctx);
const char *blank3d_skybox_recipe89_active_name(const Blank3DSkyboxRecipe89 *ctx);

#ifdef __cplusplus
}
#endif

#endif
