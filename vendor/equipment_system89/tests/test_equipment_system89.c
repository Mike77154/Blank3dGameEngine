#include "equipment_system89.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c,m) do { if (!(c)) { printf("FAIL: %s\n", (m)); return 1; } } while (0)

typedef struct HostTag { int attached; int triggered; } Host;
static int resolve(void *u, int item, EQ89_ItemDesc *d) { (void)u; memset(d,0,sizeof(*d)); d->item_id=item; d->model_id=item+10; strcpy(d->socket_name,"hand"); strcpy(d->attachment_name,"held"); strcpy(d->model_name,"test"); return 1; }
static int define_socket(void *u,int a,const char*n,int i,const void*o){(void)u;(void)a;(void)n;(void)i;(void)o;return 1;}
static int attach(void *u,int a,int o,const char*n,const char*s,const void*l){Host*h=(Host*)u;(void)a;(void)o;(void)n;(void)s;(void)l;h->attached=1;return 1;}
static int detach(void *u,int a,int o,const char*n){Host*h=(Host*)u;(void)a;(void)o;(void)n;h->attached=0;return 1;}
static int visible(void *u,int a,int o,const char*n,int v){(void)u;(void)a;(void)o;(void)n;(void)v;return 1;}
static void trigger(void *u,int slot,int event){Host*h=(Host*)u;(void)slot;(void)event;h->triggered=1;}
int main(void){EQ89_System s;EQ89_Provider p;Host h;memset(&p,0,sizeof(p));memset(&h,0,sizeof(h));p.user=&h;p.resolve_item=resolve;p.define_socket=define_socket;p.attach=attach;p.detach=detach;p.set_visible=visible;p.runtime_trigger=trigger;eq89_init(&s,20000,&p);CHECK(s.initialized,"init");CHECK(eq89_define_socket(&s,1,"hand",1,0),"socket");CHECK(eq89_equip(&s,1,1,7),"equip");CHECK(h.attached,"provider attach");CHECK(eq89_model_id(&s,1)==17,"model id");eq89_trigger(&s,1,7,1);CHECK(h.triggered,"runtime trigger");CHECK(eq89_unequip(&s,1),"unequip");CHECK(!h.attached,"provider detach");puts("equipment_system89: OK");return 0;}
