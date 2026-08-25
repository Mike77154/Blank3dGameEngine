#include "gveh_tank_armor.h"

void gveh_tank_armor_default(gveh_tank_armor_cfg *c)
{
    c->front = gveh_fx_from_int(150);
    c->side = gveh_fx_from_int(70);
    c->rear = gveh_fx_from_int(45);
    c->top = gveh_fx_from_int(25);
    c->slope_front = GVEH_FX_ONE / 3;
    c->slope_side = GVEH_FX_ONE / 8;
    c->spaced = 0;
    c->era = 0;
}

void gveh_tank_armor_modern(gveh_tank_armor_cfg *c)
{
    gveh_tank_armor_default(c);
    c->front = gveh_fx_from_int(520);
    c->side = gveh_fx_from_int(150);
    c->rear = gveh_fx_from_int(70);
    c->top = gveh_fx_from_int(45);
    c->slope_front = GVEH_FX_ONE / 2;
    c->spaced = gveh_fx_from_int(60);
}

void gveh_tank_armor_t72(gveh_tank_armor_cfg *c)
{
    gveh_tank_armor_modern(c);
    c->front = gveh_fx_from_int(430);
    c->side = gveh_fx_from_int(100);
    c->era = gveh_fx_from_int(80);
}

void gveh_tank_armor_tiger(gveh_tank_armor_cfg *c)
{
    gveh_tank_armor_default(c);
    c->front = gveh_fx_from_int(125);
    c->side = gveh_fx_from_int(85);
    c->rear = gveh_fx_from_int(80);
    c->top = gveh_fx_from_int(30);
    c->slope_front = GVEH_FX_ONE / 12;
}

void gveh_tank_armor_sherman(gveh_tank_armor_cfg *c)
{
    gveh_tank_armor_default(c);
    c->front = gveh_fx_from_int(95);
    c->side = gveh_fx_from_int(55);
    c->rear = gveh_fx_from_int(40);
    c->top = gveh_fx_from_int(20);
    c->slope_front = GVEH_FX_ONE / 4;
}

gveh_tank_hit gveh_tank_armor_resolve(const gveh_tank_armor_cfg *c, const gveh_tank_ammo *ammo, gveh_fx pen, gveh_i16 face, gveh_fx impact_angle)
{
    gveh_tank_hit h;
    gveh_fx base;
    gveh_fx slope;
    h.face = face;
    h.ricochet = 0;
    h.penetrated = 0;
    h.effective = 0;
    h.residual_pen = 0;
    h.spall = 0;
    h.shock = ammo->blast / 3;
    base = c->front;
    slope = c->slope_front;
    if (face == GVEH_TANK_FACE_SIDE) { base = c->side; slope = c->slope_side; }
    if (face == GVEH_TANK_FACE_REAR) { base = c->rear; slope = 0; }
    if (face == GVEH_TANK_FACE_TOP) { base = c->top; slope = 0; }
    h.effective = base + gveh_fx_mul(base, slope) + impact_angle;
    if (ammo->kind == GVEH_TANK_AMMO_HEAT) h.effective += c->spaced;
    if (ammo->kind == GVEH_TANK_AMMO_HEAT || ammo->kind == GVEH_TANK_AMMO_HESH) h.effective += c->era;
    if (impact_angle > gveh_fx_from_int(65) && ammo->kind != GVEH_TANK_AMMO_HEAT) h.ricochet = 1;
    if (h.ricochet == 0 && pen > h.effective) {
        h.penetrated = 1;
        h.residual_pen = pen - h.effective;
        h.spall = ammo->spall + (h.residual_pen / 2);
        h.shock = ammo->blast + (h.spall / 4);
    } else {
        h.shock = ammo->blast / 2;
    }
    return h;
}
