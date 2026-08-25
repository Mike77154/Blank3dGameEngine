#include "gprojectilespawn89.h"
#include <assert.h>
#include <stdio.h>
typedef struct T{int w,c,r,a,p;}T;
static int w(void*u,const gps89_request*q,gps89_handle*h){T*t=(T*)u;(void)q;t->w++;h->slot=2;return 1;}
static void c(void*u,const gps89_request*q,const gps89_handle*h){((T*)u)->c++;(void)q;(void)h;}
static void r(void*u,const gps89_request*q,const gps89_handle*h){((T*)u)->r++;(void)q;(void)h;}
static void a(void*u,const gps89_request*q,const gps89_handle*h){((T*)u)->a++;(void)q;(void)h;}
static void p(void*u,const gps89_request*q,const gps89_handle*h){((T*)u)->p++;(void)q;(void)h;}
int main(void){T t={0,0,0,0,0};gps89_runtime rt;gps89_providers ps;gps89_request q;gps89_handle h;ps.user=&t;ps.world_spawn=w;ps.collision_register=c;ps.render_spawn=r;ps.audio_spawn=a;ps.world_publish=p;ps.world_release=0;q.weapon_id=1;gprojectilespawn89_init(&rt,&ps);assert(gprojectilespawn89_spawn(&rt,&q,&h));assert(h.slot==2&&t.w&&t.c&&t.r&&t.a&&t.p);puts("gprojectilespawn89: OK");return 0;}
