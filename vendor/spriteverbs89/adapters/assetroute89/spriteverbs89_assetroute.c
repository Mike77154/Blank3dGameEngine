#include "spriteverbs89_assetroute.h"
int sv89_assetroute_lazy(void *user, SpriteAsset89 *assets, const char *logical_name, sa89_id *out_asset_id)
{
    SV89_AssetRouteAdapter *a;
    char path[AR89_PATH_CAP];
    sa89_id id;
    a=(SV89_AssetRouteAdapter*)user;
    if(!a||!a->routes||!assets||!logical_name||!out_asset_id)return 0;
    if(!ar89_resolve_name(a->routes,AR89_KIND_IMAGE,logical_name,path,AR89_PATH_CAP))return 0;
    id=sa89_find_asset(assets,logical_name);
    if(id==SA89_INVALID_ID){if(!sa89_define_static(assets,logical_name,path,a->static_duration_ms?a->static_duration_ms:1000U))return 0;id=sa89_find_asset(assets,logical_name);}
    if(id==SA89_INVALID_ID) return 0;
    *out_asset_id=id;
    return 1;
}
