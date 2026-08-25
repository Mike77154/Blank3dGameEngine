#include "genvidverbs89.h"
#include <stdio.h>
static int apply_cb(void*u,const gvc89_config*c,const gvc89_rect*r,int ow,int oh){(void)u;(void)c;(void)r;(void)ow;(void)oh;return 1;}
int main(void){gvc89_state s;gvc89_provider p;const char*r[2]={"320","180"};const char*m[1]={"integer"};gvc89_defaults(&s);p.user=0;p.apply_video=apply_cb;gvc89_set_provider(&s,&p);if(gvvrb89_perform(&s,"video_resolution",r,2)!=GVVRB89_HANDLED)return 1;if(gvvrb89_perform(&s,"screen_scale",m,1)!=GVVRB89_HANDLED)return 2;puts("GenVidVerbs89: PASS");return 0;}
