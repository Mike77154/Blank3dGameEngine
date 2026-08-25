#ifndef SPRITEVERBS89_ASEPRITE_H
#define SPRITEVERBS89_ASEPRITE_H
#include "spriteasset89.h"
int sv89_aseprite_import_json(SpriteAsset89 *assets, const char *asset_name,
                              const char *sheet_path_override,
                              const char *json, unsigned int json_size,
                              sa89_id *out_asset_id);
#endif
