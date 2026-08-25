#include "gmplyss89.h"
#include <stdio.h>
static int apply_cb(void*u,const gpss89_config*c){(void)u;(void)c;return 1;}
int main(void){gpss89_state s;gpss89_provider p;const char*c[2]={"320","180"};const char*r[2]={"4000","2000"};int truth;gpss89_defaults(&s);p.user=0;p.apply_gameplay_screen=apply_cb;gpss89_set_provider(&s,&p);if(gmplyss89_perform(&s,"scene_screen_size",r,2)!=GMPLYSS89_HANDLED)return 1;if(gmplyss89_perform(&s,"camera_size_draw",c,2)!=GMPLYSS89_HANDLED)return 2;if(gmplyss89_query(&s,"camera_fits_scene",&truth)!=GMPLYSS89_HANDLED||!truth)return 3;puts("GmplySS89: PASS");return 0;}
