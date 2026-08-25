#include "gcasingruntime89.h"
#include <assert.h>
#include <stdio.h>
static int w(void*u,const gcr89_request*q,gcr89_handle*h){int*x=(int*)u;(void)q;(*x)++;h->slot=3;return 1;}
static void stage(void*u,const gcr89_request*q,const gcr89_handle*h){int*x=(int*)u;(void)q;(void)h;(*x)++;}
int main(void){int n=0;gcr89_runtime r;gcr89_providers ps;gcr89_request q;gcr89_handle h;ps.user=&n;ps.world_spawn=w;ps.physics_spawn=ps.render_spawn=ps.audio_spawn=ps.world_publish=stage;ps.world_release=0;gcasingruntime89_init(&r,&ps);assert(gcasingruntime89_spawn(&r,&q,&h));assert(n==5&&h.slot==3);puts("gcasingruntime89: OK");return 0;}
