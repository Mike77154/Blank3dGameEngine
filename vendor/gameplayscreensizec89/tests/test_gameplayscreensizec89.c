#include "gameplayscreensizec89.h"
#include <stdio.h>
static int apply_cb(void*u,const gpss89_config*c){(void)u;return c->camera_draw_width==320&&c->scene_screen_width==4000;}
int main(void){gpss89_state s;gpss89_provider p;gpss89_defaults(&s);if(!gpss89_set_scene_screen_size(&s,4000,2000)||!gpss89_set_camera_draw_size(&s,320,180))return 1;if(!gpss89_set_camera_position(&s,3999,1999))return 2;if(s.config.camera_x!=3680||s.config.camera_y!=1820)return 3;p.user=0;p.apply_gameplay_screen=apply_cb;gpss89_set_provider(&s,&p);if(!gpss89_apply(&s))return 4;puts("GameplayScreenSizeC89: PASS");return 0;}
