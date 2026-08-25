#include "gprojectilespawn89.h"
#include <string.h>
void gprojectilespawn89_init(gps89_runtime *r,const gps89_providers *p){if(!r)return;memset(r,0,sizeof(*r));if(p)r->providers=*p;}
int gprojectilespawn89_spawn(gps89_runtime *r,const gps89_request *q,gps89_handle *out)
{
    gps89_handle h;
    if(!r||!q||!r->providers.world_spawn){if(r)r->rejected++;return 0;}
    memset(&h,0,sizeof(h));h.slot=-1;
    if(!r->providers.world_spawn(r->providers.user,q,&h)){r->rejected++;return 0;}
    if(r->providers.collision_register)r->providers.collision_register(r->providers.user,q,&h);
    if(r->providers.render_spawn)r->providers.render_spawn(r->providers.user,q,&h);
    if(r->providers.audio_spawn)r->providers.audio_spawn(r->providers.user,q,&h);
    if(r->providers.world_publish)r->providers.world_publish(r->providers.user,q,&h);
    r->accepted++;
    if(out)*out=h;
    return 1;
}
