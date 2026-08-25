#include "gweaponsnapshot89.h"
#include <string.h>
void gweaponsnapshot89_init(gws89_store *store){ if(store) memset(store,0,sizeof(*store)); }
int gweaponsnapshot89_publish(gws89_store *store,const gws89_snapshot *s)
{
    int i,free_slot;
    if(!store||!s||s->actor_id<=0) return 0;
    free_slot=-1;
    for(i=0;i<GWS89_CAPACITY;++i){
        if(store->entries[i].valid && store->entries[i].actor_id==s->actor_id){ store->entries[i]=*s; store->entries[i].valid=1; store->writes++; return 1; }
        if(free_slot<0 && !store->entries[i].valid) free_slot=i;
    }
    if(free_slot<0) free_slot=s->actor_id % GWS89_CAPACITY;
    store->entries[free_slot]=*s; store->entries[free_slot].valid=1; store->writes++; return 1;
}
int gweaponsnapshot89_get(const gws89_store *store,int actor_id,unsigned long frame_id,gws89_snapshot *out)
{
    int i; if(!store||!out) return 0;
    for(i=0;i<GWS89_CAPACITY;++i) if(store->entries[i].valid && store->entries[i].actor_id==actor_id && store->entries[i].frame_id==frame_id){ *out=store->entries[i]; return 1; }
    return 0;
}
