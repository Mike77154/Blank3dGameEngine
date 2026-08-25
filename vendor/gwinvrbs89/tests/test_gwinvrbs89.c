#include "gwinvrbs89.h"
#include <stdio.h>
static int apply_cb(void*u,const gwc89_config*c){(void)u;(void)c;return 1;}
int main(void){gwc89_state s;gwc89_provider p;const char*a[2]={"640","360"};int truth;gwc89_defaults(&s);p.user=0;p.create_window=0;p.apply_window=apply_cb;p.destroy_window=0;p.query_client_size=0;gwc89_set_provider(&s,&p);if(gwvrb89_perform(&s,"window_size",a,2)!=GWVRB89_HANDLED)return 1;if(s.config.client_width!=640)return 2;if(gwvrb89_query(&s,"window_is_windowed",&truth)!=GWVRB89_HANDLED||!truth)return 3;puts("GWinVrbs89: PASS");return 0;}
