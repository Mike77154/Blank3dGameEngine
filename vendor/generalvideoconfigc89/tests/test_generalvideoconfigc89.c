#include "generalvideoconfigc89.h"
#include <stdio.h>
static int apply_cb(void*u,const gvc89_config*c,const gvc89_rect*r,int ow,int oh){(void)u;return c->resolution_width==320&&r->width==1280&&r->height==720&&ow==1366&&oh==768;}
int main(void){gvc89_state s;gvc89_provider p;gvc89_defaults(&s);if(!gvc89_load_text(&s,"[Video]\nResolution=320x180\nScreenScale=integer\nFilter=nearest\n"))return 1;if(!gvc89_set_output_size(&s,1366,768))return 2;if(s.presentation.width!=1280||s.presentation.height!=720||s.presentation.x!=43||s.presentation.y!=24)return 3;p.user=0;p.apply_video=apply_cb;gvc89_set_provider(&s,&p);if(!gvc89_apply(&s))return 4;puts("GeneralVideoConfigC89: PASS");return 0;}
