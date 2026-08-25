#include "blank3d_bolt.h"
#include <string.h>
static int b3d_bolt_sweep(void *user,const GWP89_Vec3 *start,const GWP89_Vec3 *delta,gwp89_fx radius,GBoltSweepHit89 *out){Blank3DCollisionHit hit;if(!user||!out)return 0;memset(&hit,0,sizeof(hit));hit.enemy_index=-1;if(!blank3d_collision_sweep_bullet((Blank3DCollision*)user,start,delta,radius,&hit))return 0;memset(out,0,sizeof(*out));out->hit=hit.hit;out->target_id=hit.enemy_index;out->target_layer=hit.enemy_index>=0?B3D_LAYER_ENEMY:B3D_LAYER_WORLD;out->material_id=hit.material_id;out->fraction_fx=hit.fraction_fx;out->point=hit.point;out->normal=hit.normal;return 1;}
void blank3d_bolt_init(Blank3DBolt *b,Blank3DCollision *c){gboltweapon89_init(b,b3d_bolt_sweep,c);}
int blank3d_bolt_spawn(Blank3DBolt *b,const Blank3DWeaponModules *m,int o,const GWP89_Vec3 *p,const GWP89_Vec3 *d,gwp89_fx s,gwp89_fx r,gwp89_fx dmg,unsigned short life,void *u){return gboltweapon89_spawn(b,m,o,p,d,s,r,dmg,life,u);}
void blank3d_bolt_update(Blank3DBolt *b,gwp89_fx dt){gboltweapon89_update(b,dt);}
const B3D_Projectile *blank3d_bolt_get(const Blank3DBolt *b,int id){return gboltweapon89_get(b,id);}
int blank3d_bolt_poll_event(Blank3DBolt *b,B3D_Event *e){return gboltweapon89_poll_event(b,e);}
void blank3d_bolt_destroy(Blank3DBolt *b,int id){gboltweapon89_destroy(b,id);}
