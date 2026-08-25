#ifndef BLANK3D_DISPLAY_STACK89_H
#define BLANK3D_DISPLAY_STACK89_H

#include "genwinconfigc89.h"
#include "generalvideoconfigc89.h"
#include "gameplayscreensizec89.h"
#include "gwinvrbs89.h"
#include "genvidverbs89.h"
#include "gmplyss89.h"
#include "gameverbs89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_GENERALCFG_TEXT_CAP 8192

typedef struct Blank3DDisplayStack89Tag {
    gwc89_state window;
    gvc89_state video;
    gpss89_state gameplay;
    char general_cfg_text[B3D_GENERALCFG_TEXT_CAP];
    char general_cfg_path[160];
    char status[160];
    int initialized;
} Blank3DDisplayStack89;

void blank3d_display_stack89_init(Blank3DDisplayStack89 *stack);
void blank3d_display_stack89_set_window_provider(
    Blank3DDisplayStack89 *stack, const gwc89_provider *provider);
void blank3d_display_stack89_set_video_provider(
    Blank3DDisplayStack89 *stack, const gvc89_provider *provider);
void blank3d_display_stack89_set_gameplay_provider(
    Blank3DDisplayStack89 *stack, const gpss89_provider *provider);
int blank3d_display_stack89_load_general_cfg(
    Blank3DDisplayStack89 *stack, const char *path);
int blank3d_display_stack89_set_gameplay_sizes(
    Blank3DDisplayStack89 *stack,
    int camera_width, int camera_height,
    int scene_width, int scene_height);
int blank3d_display_stack89_apply_video_gameplay(
    Blank3DDisplayStack89 *stack);
int blank3d_display_stack89_create_window(Blank3DDisplayStack89 *stack);
int blank3d_display_stack89_register_verbs(
    Blank3DDisplayStack89 *stack, gverb89_registry *registry);
int blank3d_display_stack89_window_resized(
    Blank3DDisplayStack89 *stack, int width, int height);
const char *blank3d_display_stack89_status(
    const Blank3DDisplayStack89 *stack);

#ifdef __cplusplus
}
#endif
#endif
