#include "gweaponsnapshot89.h"
#include <assert.h>
#include <stdio.h>
int main(void){gws89_store st;gws89_snapshot a,b;gweaponsnapshot89_init(&st);a.valid=1;a.actor_id=7;a.frame_id=44;a.muzzle_origin.x=9;assert(gweaponsnapshot89_publish(&st,&a));assert(gweaponsnapshot89_get(&st,7,44,&b));assert(b.muzzle_origin.x==9);assert(!gweaponsnapshot89_get(&st,7,43,&b));puts("gweaponsnapshot89: OK");return 0;}
