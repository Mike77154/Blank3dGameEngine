#ifndef GSYNTHSOUNDEXPANSION89_H
#define GSYNTHSOUNDEXPANSION89_H
#include "wsoundmuzzledevice89.h"
#include "wsoundaero89.h"
#include "wsoundfriction89.h"
#include "wsoundparticles89.h"
#include "wsoundammo89.h"
#include "wsoundbeltfeed89.h"
#include "wsoundoutdoor89.h"
#include "wsoundportal89.h"
#include "wsounddoppler89.h"
#include "wsoundspatial89.h"
#include "wsoundthermal89.h"
#include "wsoundlistener89.h"
#include "wsoundmask89.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GSSEXP89_MUZZLE       0x0001U
#define GSSEXP89_AERO         0x0002U
#define GSSEXP89_FRICTION     0x0004U
#define GSSEXP89_PARTICLES    0x0008U
#define GSSEXP89_AMMO         0x0010U
#define GSSEXP89_BELT         0x0020U
#define GSSEXP89_OUTDOOR      0x0040U
#define GSSEXP89_PORTAL       0x0080U
#define GSSEXP89_DOPPLER      0x0100U
#define GSSEXP89_SPATIAL      0x0200U
#define GSSEXP89_THERMAL      0x0400U
#define GSSEXP89_LISTENER     0x0800U
#define GSSEXP89_MASK         0x1000U
#define GSSEXP89_ALL          0x1FFFU
#define GSSEXP89_SAFE_DEFAULT (GSSEXP89_MUZZLE|GSSEXP89_PARTICLES|GSSEXP89_AMMO|GSSEXP89_OUTDOOR|GSSEXP89_SPATIAL|GSSEXP89_THERMAL|GSSEXP89_MASK)

typedef struct gssexp89_context_s {
    wsound89_u32 sample_rate;
    wsound89_u16 enabled_mask;
    wsound89_u16 outdoor_send_q15;
    wsound89_u16 outdoor_wet_q15;
    wsound89_u32 outdoor_send_remaining;
    wsound89_u32 outdoor_send_total;
    wsoundmuzzledevice89_context muzzle;
    wsoundaero89_context aero;
    wsoundfriction89_context friction;
    wsoundparticles89_context particles;
    wsoundammo89_context ammo;
    wsoundbeltfeed89_context belt;
    wsoundoutdoor89_context outdoor;
    wsoundportal89_context portal;
    wsounddoppler89_context doppler;
    wsoundspatial89_context spatial;
    wsoundthermal89_context thermal;
    wsoundlistener89_context listener;
    wsoundmask89_context mask;
} gssexp89_context;

typedef struct gssexp89_memory_s {
    wsound89_i16 *outdoor;
    wsound89_u32 outdoor_frames;
    wsound89_i16 *portal;
    wsound89_u32 portal_frames;
    wsound89_i16 *spatial_left;
    wsound89_i16 *spatial_right;
    wsound89_u16 spatial_frames;
} gssexp89_memory;

wsound89_result gssexp89_init(gssexp89_context*,wsound89_u32,wsound89_u32,const gssexp89_memory*);
void gssexp89_enable(gssexp89_context*,wsound89_u16);
void gssexp89_set_outdoor_mix(gssexp89_context*,wsound89_u16,wsound89_u16);
void gssexp89_trigger_shot(gssexp89_context*,wsoundmuzzledevice89_type,wsound89_u16,wsound89_u16,wsound89_u32);
void gssexp89_start_belt(gssexp89_context*,wsound89_u16,wsound89_u16);
void gssexp89_stop_belt(gssexp89_context*);
void gssexp89_start_friction(gssexp89_context*,wsoundfriction89_material,wsound89_u16,wsound89_u16,wsound89_u16);
void gssexp89_stop_friction(gssexp89_context*);
void gssexp89_start_aero(gssexp89_context*,wsoundaero89_mode,wsound89_u16,wsound89_u16,wsound89_u32);
void gssexp89_process_mono(gssexp89_context*,wsound89_i16,wsound89_i16*,wsound89_i16*);
wsound89_u32 gssexp89_context_bytes(void);
#ifdef __cplusplus
}
#endif
#endif
