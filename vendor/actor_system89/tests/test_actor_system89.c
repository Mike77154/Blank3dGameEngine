#include "actor_system89.h"
#include <stdio.h>
#include <string.h>

typedef struct HostTag { int alive; AS89_Position pos; } Host;
static int q_alive(void *u, int id, int stored) { Host *h=(Host*)u; (void)id; return h ? h->alive : stored; }
static int q_pos(void *u, int id, AS89_Position *out) { Host *h=(Host*)u; (void)id; if(!h||!out) return 0; *out=h->pos; return 1; }
int main(void)
{
    AS89_System s; AS89_Provider p; Host h; AS89_Position pos;
    memset(&h,0,sizeof(h)); h.alive=1; h.pos.x=10;
    as89_init(&s); memset(&p,0,sizeof(p)); p.user=&h; p.query_alive=q_alive; p.query_position=q_pos; as89_set_provider(&s,&p);
    if(as89_register(&s,7,0x1234UL,44UL,1,1,0UL)<0) return 1;
    if(as89_owner_entity(&s,7)!=0x1234UL || as89_user_ref(&s,7)!=44UL) return 2;
    if(!as89_is_alive(&s,7) || !as89_position(&s,7,&pos) || pos.x!=10) return 3;
    if(!as89_set_owner_entity(&s,7,0x7777UL) || as89_owner_entity(&s,7)!=0x7777UL) return 4;
    puts("actor_system89 agnostic actor/entity-link test: OK");
    return 0;
}
