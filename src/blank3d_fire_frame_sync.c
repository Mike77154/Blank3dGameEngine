#include "blank3d_fire_frame_sync.h"
#include <string.h>
static int b3d_ff_view(void *user,GWP89_CameraState *out){Blank3DCameraNaku *c=(Blank3DCameraNaku*)user;Vec3 e,f,r,u;if(!c||!out)return 0;blank3d_cameranaku_get_view(c,&e,&f,&r,&u);memset(out,0,sizeof(*out));out->valid=1;out->origin.x=e.x;out->origin.y=e.y;out->origin.z=e.z;out->forward.x=f.x;out->forward.y=f.y;out->forward.z=f.z;out->right.x=r.x;out->right.y=r.y;out->right.z=r.z;out->up.x=u.x;out->up.y=u.y;out->up.z=u.z;return 1;}
static void b3d_ff_recoil(void *user,gwp89_fx pitch,gwp89_fx shake){blank3d_cameranaku_add_recoil((Blank3DCameraNaku*)user,(g3d_fix)pitch,(g3d_fix)shake);}
void blank3d_fire_frame_sync_init(Blank3DFireFrameSync *s){gfireframe89_init(s);}
void blank3d_fire_frame_sync_queue_recoil(Blank3DFireFrameSync *s,g3d_fix p,g3d_fix sh){gfireframe89_queue_recoil(s,(gwp89_fx)p,(gwp89_fx)sh);}
unsigned int blank3d_fire_frame_sync_apply_pending(Blank3DFireFrameSync *s,Blank3DCameraNaku *c){return gfireframe89_apply_pending(s,b3d_ff_recoil,c);}
void blank3d_fire_frame_sync_capture(Blank3DFireFrameSync *s,unsigned int id,const Blank3DCameraNaku *c){(void)gfireframe89_capture(s,id,b3d_ff_view,(void*)c);}
int blank3d_fire_frame_sync_get_view(const Blank3DFireFrameSync *s,unsigned int id,Vec3 *e,Vec3 *f,Vec3 *r,Vec3 *u){GWP89_Vec3 qe,qf,qr,qu;if(!gfireframe89_get_view(s,id,&qe,&qf,&qr,&qu))return 0;if(e)*e=gamlib_vec3(qe.x,qe.y,qe.z);if(f)*f=gamlib_vec3(qf.x,qf.y,qf.z);if(r)*r=gamlib_vec3(qr.x,qr.y,qr.z);if(u)*u=gamlib_vec3(qu.x,qu.y,qu.z);return 1;}
