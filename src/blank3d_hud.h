#ifndef BLANK3D_HUD_H
#define BLANK3D_HUD_H

#include "gbar89.h"
#include "blank3d_sniper.h"
#include "blank3d_ecg_vitals.h"
#include "blank3d_bighud.h"
#include "blank3d_crosshair.h"
#include "blank3d_gproj_ammo_gbar.h"
#include "blank3d_text89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_HUD_CLIP_STACK_MAX 8

struct Blank3DNumbarSystemTag;

typedef void (*Blank3DHudSpriteDrawFn)(void *user,
                                       int sprite_id,
                                       int sx, int sy, int sw, int sh,
                                       int dx, int dy, int dw, int dh,
                                       unsigned long tint_rgba);

typedef void (*Blank3DHudSpriteUvDrawFn)(void *user,
                                         int sprite_id,
                                         int u0, int v0, int u1, int v1,
                                         int dx, int dy, int dw, int dh,
                                         unsigned long tint_rgba);

typedef struct Blank3DHudSpriteProviderTag {
    void *user;
    Blank3DHudSpriteDrawFn draw;
    Blank3DHudSpriteUvDrawFn draw_uv;
} Blank3DHudSpriteProvider;

typedef struct Blank3DHudTag {
    GBar89_RenderOps render_ops;
    Blank3DHudSpriteProvider sprite_provider;
    Blank3DEcgVitals ecg;
    Blank3DBigHud layout;
    Blank3DCrosshair crosshair;
    Blank3DGProjAmmoGBar ammo_gbar;
    Blank3DText89 *text;
    int crosshair_weapon_id;
    unsigned char ecg_rgba[B3D_ECG_WIDTH * B3D_ECG_HEIGHT * 4u];
    int width;
    int height;
    int clip_depth;
    int clip_x[B3D_HUD_CLIP_STACK_MAX];
    int clip_y[B3D_HUD_CLIP_STACK_MAX];
    int clip_w[B3D_HUD_CLIP_STACK_MAX];
    int clip_h[B3D_HUD_CLIP_STACK_MAX];
} Blank3DHud;

void blank3d_hud_init(Blank3DHud *hud);
void blank3d_hud_set_sprite_provider(Blank3DHud *hud,
                                     const Blank3DHudSpriteProvider *provider);
void blank3d_hud_set_weapon_ammo_id(Blank3DHud *hud, int ammo_id);
void blank3d_hud_set_text_provider(Blank3DHud *hud, Blank3DText89 *text);
const char *blank3d_hud_text_status(const Blank3DHud *hud);
void blank3d_hud_draw_layout_scaled(Blank3DHud *hud,
                                      Blank3DBigHud *layout,
                                      const Blank3DBigHudTelemetry *telemetry,
                                      unsigned int frame_ms,
                                      int anchor,
                                      int offset_x,
                                      int offset_y,
                                      int scale_x_q8,
                                      int scale_y_q8,
                                      int canvas_width,
                                      int canvas_height);

void blank3d_hud_draw(Blank3DHud *hud,
                      struct Blank3DNumbarSystemTag *numbars,
                      int width,
                      int height,
                      int player_health,
                      int clip,
                      int clip_capacity,
                      int reserve,
                      int weapon_id,
                      int first_person,
                      int aiming,
                      int muzzle_flash,
                      int threat_level,
                      unsigned int damage_flash_ms,
                      unsigned int frame_ms,
                      Blank3DSniper *sniper);

#ifdef __cplusplus
}
#endif

#endif
