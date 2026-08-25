#include "genwinconfigc89.h"
#include <stdio.h>
static int created,applied;
static int create_cb(void*u,const gwc89_config*c){(void)u;created++;return c->client_width==1280&&c->play_mode==GWC89_MODE_WINDOWED;}
static int apply_cb(void*u,const gwc89_config*c){(void)u;(void)c;applied++;return 1;}
static int query_cb(void*u,int*w,int*h){(void)u;*w=1280;*h=720;return 1;}
int main(void){gwc89_state s;gwc89_provider p;const char*t="[Window]\nWindowSize=1280x720\nPlayMode=windowed\nResizable=1\nTitle=Hello\n";gwc89_defaults(&s);p.user=0;p.create_window=create_cb;p.apply_window=apply_cb;p.destroy_window=0;p.query_client_size=query_cb;gwc89_set_provider(&s,&p);if(!gwc89_load_text(&s,t)||!gwc89_create(&s))return 1;if(created!=1||s.observed_client_width!=1280)return 2;if(!gwc89_set_play_mode(&s,GWC89_MODE_FULLSCREEN)||!gwc89_apply(&s)||applied!=1)return 3;puts("GenWinConfigC89: PASS");return 0;}
