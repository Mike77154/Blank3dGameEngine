#include "condor_evact89.h"
#include <stdio.h>
#include <string.h>

typedef struct TStateTag { int flag; int actions; int listeners; } TState;
static int cond_true(const cea89_event *e, void *u)
{ TState *s=(TState*)u; (void)e; return s && s->flag; }
static void action_inc(const cea89_event *e, void *u)
{ TState *s=(TState*)u; (void)e; if(s) ++s->actions; }
static void listen_inc(const cea89_event *e, void *u)
{ TState *s=(TState*)u; (void)e; if(s) ++s->listeners; }
int main(void)
{
    cea89_context a, b;
    TState sa, sb;
    cea89_condition_id c;
    cea89_action_id x;
    cea89_rule_id r, stale;
    cea89_listener_id l;
    cea89_event e;
    memset(&sa,0,sizeof(sa)); memset(&sb,0,sizeof(sb));
    cea89_init(&a); cea89_init(&b);
    sa.flag=1; sb.flag=1;
    c=cea89_condition_register(&a,cond_true,&sa);
    x=cea89_action_register(&a,action_inc,&sa);
    r=cea89_rule_register(&a,7,9,c,x);
    l=cea89_subscribe(&a,7,listen_inc,&sa);
    if(!c||!x||!r||!l) return 2;
    memset(&e,0,sizeof(e)); e.type=7; e.code=9; e.owner=11; e.instance=22;
    if(cea89_emit(&a,&e)!=1 || sa.actions!=1 || sa.listeners!=1) return 3;
    if(cea89_emit(&b,&e)!=0 || sb.actions!=0) return 4;
    stale=r;
    if(!cea89_rule_unregister(&a,r)) return 5;
    r=cea89_rule_register(&a,7,9,c,x);
    if(!r || r==stale) return 6;
    if(cea89_rule_unregister(&a,stale)) return 7;
    cea89_reset(&a);
    if(cea89_rule_count(&a)!=0 || cea89_condition_count(&a)!=0 ||
       cea89_action_count_registered(&a)!=0 || cea89_listener_count(&a)!=0) return 8;
    puts("condor_evact89: PASS");
    return 0;
}
