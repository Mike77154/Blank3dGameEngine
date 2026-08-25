#ifndef GVEH_TANK_ARMOR_H
#define GVEH_TANK_ARMOR_H

#include "gveh_tank_gun.h"

#define GVEH_TANK_FACE_FRONT 0
#define GVEH_TANK_FACE_SIDE  1
#define GVEH_TANK_FACE_REAR  2
#define GVEH_TANK_FACE_TOP   3

typedef struct gveh_tank_armor_cfg_s {
    gveh_fx front;
    gveh_fx side;
    gveh_fx rear;
    gveh_fx top;
    gveh_fx slope_front;
    gveh_fx slope_side;
    gveh_fx spaced;
    gveh_fx era;
} gveh_tank_armor_cfg;

typedef struct gveh_tank_hit_s {
    gveh_i16 face;
    gveh_i16 ricochet;
    gveh_i16 penetrated;
    gveh_fx effective;
    gveh_fx residual_pen;
    gveh_fx spall;
    gveh_fx shock;
} gveh_tank_hit;

void gveh_tank_armor_default(gveh_tank_armor_cfg *c);
void gveh_tank_armor_modern(gveh_tank_armor_cfg *c);
void gveh_tank_armor_t72(gveh_tank_armor_cfg *c);
void gveh_tank_armor_tiger(gveh_tank_armor_cfg *c);
void gveh_tank_armor_sherman(gveh_tank_armor_cfg *c);
gveh_tank_hit gveh_tank_armor_resolve(const gveh_tank_armor_cfg *c, const gveh_tank_ammo *ammo, gveh_fx pen, gveh_i16 face, gveh_fx impact_angle);

#endif
