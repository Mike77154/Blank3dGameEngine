#include "gplayerprojectileaim89.h"
#include <assert.h>
#include <stdio.h>
static int target(void *u,const gppa89_query *q,gppa89_target *t)
{ (void)u; (void)q; t->valid=1; t->hit=1; t->blocked_from_muzzle=0; t->target.x=0; t->target.y=0; t->target.z=40960; return 1; }
int main(void){ gppa89_query q; gppa89_result r; q.muzzle_origin.x=0;q.muzzle_origin.y=0;q.muzzle_origin.z=0;q.fallback_direction.x=0;q.fallback_direction.y=0;q.fallback_direction.z=4096; assert(gplayerprojectileaim89_resolve(&q,target,0,&r)); assert(r.direction.z>0); puts("gplayerprojectileaim89: OK"); return 0; }
