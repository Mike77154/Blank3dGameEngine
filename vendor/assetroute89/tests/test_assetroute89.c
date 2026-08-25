#include "assetroute89.h"
#include <stdio.h>
#include <string.h>

static const char *files[] = {
 "game/images/Eileen Happy.PNG",
 "game/images/sub/eileen happy.jpg",
 "game/images/bg meadow.webp",
 "game/audio/Town_theme.ogg",
 "game/audio/my song.mp3",
 "game/audio/z/Town_theme.mp3"
};
static int fake_exists(void *u,const char*p){unsigned int i;(void)u;for(i=0U;i<sizeof(files)/sizeof(files[0]);++i)if(strcmp(files[i],p)==0)return AR89_PROVIDER_FOUND;return AR89_PROVIDER_MISSING;}
static int fake_enum(void*u,const char*root,int rec,ar89_enum_sink_fn sink,void*su){unsigned int i;(void)u;(void)root;(void)rec;for(i=0U;i<sizeof(files)/sizeof(files[0]);++i)if(!sink(su,files[i]))return AR89_PROVIDER_ERROR;return AR89_PROVIDER_FOUND;}
int main(void){AssetRoute89 a;AR89_FileProvider p;char out[AR89_PATH_CAP];ar89_init(&a);p.exists=fake_exists;p.enumerate=fake_enum;p.user=0;ar89_set_file_provider(&a,&p);ar89_add_root(&a,AR89_KIND_IMAGE,"game/images",1);ar89_add_root(&a,AR89_KIND_AUDIO,"game/audio",1);if(!ar89_scan_roots(&a))return 2;if(!ar89_resolve_name(&a,AR89_KIND_IMAGE,"eileen happy",out,sizeof(out)))return 3;if(strcmp(out,"game/images/Eileen Happy.PNG")!=0)return 4;if(!ar89_resolve_name(&a,AR89_KIND_AUDIO,"town_theme",out,sizeof(out)))return 5;if(strcmp(out,"game/audio/Town_theme.ogg")!=0)return 6;if(ar89_resolve_name(&a,AR89_KIND_AUDIO,"my song",out,sizeof(out)))return 7;if(!ar89_register_explicit(&a,AR89_KIND_IMAGE,"eileen happy","explicit/eileen.png"))return 8;if(!ar89_resolve_name(&a,AR89_KIND_IMAGE,"eileen happy",out,sizeof(out)))return 9;if(strcmp(out,"explicit/eileen.png")!=0)return 10;printf("AssetRoute89 PASS entries=%u\n",(unsigned)a.entry_count);return 0;}
