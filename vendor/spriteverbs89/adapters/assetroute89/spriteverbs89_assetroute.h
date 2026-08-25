#ifndef SPRITEVERBS89_ASSETROUTE_H
#define SPRITEVERBS89_ASSETROUTE_H
#include "spriteverbs89.h"
#include "assetroute89.h"
typedef struct SV89_AssetRouteAdapter_s { AssetRoute89 *routes; unsigned int static_duration_ms; } SV89_AssetRouteAdapter;
int sv89_assetroute_lazy(void *user, SpriteAsset89 *assets, const char *logical_name, sa89_id *out_asset_id);
#endif
