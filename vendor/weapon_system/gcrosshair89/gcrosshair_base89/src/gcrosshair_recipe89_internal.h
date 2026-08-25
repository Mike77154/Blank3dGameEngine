#ifndef GCROSSHAIR_RECIPE89_INTERNAL_H
#define GCROSSHAIR_RECIPE89_INTERNAL_H

#include "gcrosshair_base89.h"

int gcb89_recipe__ensure(void);
int gcb89_recipe__count(void);
int gcb89_recipe__valid(int preset_id);
const char *gcb89_recipe__name(int preset_id);
const char *gcb89_recipe__category(int preset_id);
const GC89_Style *gcb89_recipe__style(int preset_id);
const GCB89_AnimationPreset *gcb89_recipe__animation(int preset_id);
const GCB89_AnimationRecipe *gcb89_recipe__animation_recipe(int preset_id);
const char *gcb89_recipe__asset_filename(int image_id);

#endif
