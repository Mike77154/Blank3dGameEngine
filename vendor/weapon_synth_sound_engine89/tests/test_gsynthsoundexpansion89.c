#include <stdio.h>
#include "gsynthsoundexpansion89.h"
#define RATE 44100U
static wsound89_i16 outdoor[RATE];static wsound89_i16 portal[RATE/4U];static wsound89_i16 sl[64],sr[64];
int main(void){gssexp89_context c;gssexp89_memory m;wsound89_i16 l,r;wsound89_u32 i;m.outdoor=outdoor;m.outdoor_frames=RATE;m.portal=portal;m.portal_frames=RATE/4U;m.spatial_left=sl;m.spatial_right=sr;m.spatial_frames=64U;if(gssexp89_init(&c,RATE,1U,&m)!=WSOUND89_OK)return 1;gssexp89_trigger_shot(&c,WSOUNDMUZZLEDEVICE89_BRAKE,30000U,1200U,7U);for(i=0;i<RATE;i++)gssexp89_process_mono(&c,0,&l,&r);printf("gsynthsoundexpansion89 PASS bytes=%u\n",gssexp89_context_bytes());return 0;}
