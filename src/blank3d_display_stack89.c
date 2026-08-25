#include "blank3d_display_stack89.h"

#include <stdio.h>
#include <string.h>

static void b3d_ds89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void b3d_ds89_status(Blank3DDisplayStack89 *stack, const char *text)
{
    if (!stack) return;
    b3d_ds89_copy(stack->status, sizeof(stack->status), text);
}

void blank3d_display_stack89_init(Blank3DDisplayStack89 *stack)
{
    if (!stack) return;
    memset(stack, 0, sizeof(*stack));
    gwc89_defaults(&stack->window);
    gvc89_defaults(&stack->video);
    gpss89_defaults(&stack->gameplay);
    b3d_ds89_copy(stack->general_cfg_path,
                  sizeof(stack->general_cfg_path),
                  "config/GeneralConfig.cfg");
    stack->initialized = 1;
    b3d_ds89_status(stack, "display sextet defaults");
}

void blank3d_display_stack89_set_window_provider(
    Blank3DDisplayStack89 *stack, const gwc89_provider *provider)
{
    if (!stack) return;
    gwc89_set_provider(&stack->window, provider);
}

void blank3d_display_stack89_set_video_provider(
    Blank3DDisplayStack89 *stack, const gvc89_provider *provider)
{
    if (!stack) return;
    gvc89_set_provider(&stack->video, provider);
}

void blank3d_display_stack89_set_gameplay_provider(
    Blank3DDisplayStack89 *stack, const gpss89_provider *provider)
{
    if (!stack) return;
    gpss89_set_provider(&stack->gameplay, provider);
}

int blank3d_display_stack89_load_general_cfg(
    Blank3DDisplayStack89 *stack, const char *path)
{
    FILE *file;
    long size;
    unsigned int got;
    if (!stack || !path) return 0;
    file = fopen(path, "rb");
    if (!file) {
        b3d_ds89_status(stack, "GeneralConfig.cfg missing; defaults active");
        return 0;
    }
    if (fseek(file, 0L, SEEK_END) != 0) {
        fclose(file);
        b3d_ds89_status(stack, "GeneralConfig.cfg seek failed");
        return 0;
    }
    size = ftell(file);
    if (size < 0L || size >= B3D_GENERALCFG_TEXT_CAP) {
        fclose(file);
        b3d_ds89_status(stack, "GeneralConfig.cfg too large");
        return 0;
    }
    rewind(file);
    got = (unsigned int)fread(stack->general_cfg_text, 1U,
                              (size_t)size, file);
    fclose(file);
    stack->general_cfg_text[got] = '\0';
    b3d_ds89_copy(stack->general_cfg_path,
                  sizeof(stack->general_cfg_path), path);
    if (!gwc89_load_text(&stack->window, stack->general_cfg_text) ||
        !gvc89_load_text(&stack->video, stack->general_cfg_text)) {
        b3d_ds89_status(stack, "GeneralConfig.cfg parse failed");
        return 0;
    }
    b3d_ds89_status(stack, "GeneralConfig.cfg loaded");
    return 1;
}

int blank3d_display_stack89_set_gameplay_sizes(
    Blank3DDisplayStack89 *stack,
    int camera_width, int camera_height,
    int scene_width, int scene_height)
{
    if (!stack) return 0;
    if (!gpss89_set_camera_draw_size(&stack->gameplay,
                                      camera_width, camera_height) ||
        !gpss89_set_scene_screen_size(&stack->gameplay,
                                      scene_width, scene_height))
        return 0;
    return 1;
}

int blank3d_display_stack89_apply_video_gameplay(
    Blank3DDisplayStack89 *stack)
{
    if (!stack) return 0;
    if (!gvc89_apply(&stack->video)) {
        b3d_ds89_status(stack, gvc89_status(&stack->video));
        return 0;
    }
    if (!gpss89_apply(&stack->gameplay)) {
        b3d_ds89_status(stack, gpss89_status(&stack->gameplay));
        return 0;
    }
    b3d_ds89_status(stack, "video + gameplay spaces applied");
    return 1;
}

int blank3d_display_stack89_create_window(Blank3DDisplayStack89 *stack)
{
    if (!stack) return 0;
    if (!gwc89_create(&stack->window)) {
        b3d_ds89_status(stack, gwc89_status(&stack->window));
        return 0;
    }
    (void)gvc89_set_output_size(&stack->video,
                                 stack->window.observed_client_width,
                                 stack->window.observed_client_height);
    if (!gvc89_apply(&stack->video)) {
        b3d_ds89_status(stack, gvc89_status(&stack->video));
        return 0;
    }
    b3d_ds89_status(stack, "native window created; video output synchronized");
    return 1;
}

int blank3d_display_stack89_window_resized(
    Blank3DDisplayStack89 *stack, int width, int height)
{
    if (!stack || width <= 0 || height <= 0) return 0;
    (void)gwc89_set_observed_client_size(&stack->window, width, height);
    if (!gvc89_set_output_size(&stack->video, width, height)) return 0;
    return gvc89_apply(&stack->video);
}

static int b3d_ds89_sync_window_video(Blank3DDisplayStack89 *stack)
{
    if (!stack) return 0;
    if (!gwc89_refresh(&stack->window)) return 0;
    if (!gvc89_set_output_size(&stack->video,
                                stack->window.observed_client_width,
                                stack->window.observed_client_height))
        return 0;
    return gvc89_apply(&stack->video);
}

static int b3d_ds89_action(void *user, const gverb89_call *call)
{
    Blank3DDisplayStack89 *stack;
    int result;
    stack = (Blank3DDisplayStack89 *)user;
    if (!stack || !call || !call->name) return GVERB89_UNHANDLED;
    result = gwvrb89_perform(&stack->window, call->name,
                             call->argv, call->argc);
    if (result == GWVRB89_HANDLED) {
        if (stack->window.created &&
            !b3d_ds89_sync_window_video(stack)) return GVERB89_ERROR;
        return GVERB89_HANDLED;
    }
    if (result == GWVRB89_ERROR) return GVERB89_ERROR;
    result = gvvrb89_perform(&stack->video, call->name,
                             call->argv, call->argc);
    if (result == GVVRB89_HANDLED) return GVERB89_HANDLED;
    if (result == GVVRB89_ERROR) return GVERB89_ERROR;
    result = gmplyss89_perform(&stack->gameplay, call->name,
                               call->argv, call->argc);
    if (result == GMPLYSS89_HANDLED) return GVERB89_HANDLED;
    if (result == GMPLYSS89_ERROR) return GVERB89_ERROR;
    return GVERB89_UNHANDLED;
}

static int b3d_ds89_condition(void *user, const gverb89_call *call,
                              gverb89_result *out)
{
    Blank3DDisplayStack89 *stack;
    int result;
    int truth;
    stack = (Blank3DDisplayStack89 *)user;
    if (!stack || !call || !call->name || !out) return GVERB89_UNHANDLED;
    truth = 0;
    result = gwvrb89_query(&stack->window, call->name, &truth);
    if (result == GWVRB89_ERROR) return GVERB89_ERROR;
    if (result == GWVRB89_HANDLED) {
        out->truth = truth ? 1 : 0;
        out->value_q16 = truth ? 65536L : 0L;
        out->instance_id = 0;
        return GVERB89_HANDLED;
    }
    result = gvvrb89_query(&stack->video, call->name, &truth);
    if (result == GVVRB89_ERROR) return GVERB89_ERROR;
    if (result == GVVRB89_HANDLED) {
        out->truth = truth ? 1 : 0;
        out->value_q16 = truth ? 65536L : 0L;
        out->instance_id = 0;
        return GVERB89_HANDLED;
    }
    result = gmplyss89_query(&stack->gameplay, call->name, &truth);
    if (result == GMPLYSS89_ERROR) return GVERB89_ERROR;
    if (result != GMPLYSS89_HANDLED) return GVERB89_UNHANDLED;
    out->truth = truth ? 1 : 0;
    out->value_q16 = truth ? 65536L : 0L;
    out->instance_id = 0;
    return GVERB89_HANDLED;
}

static int b3d_ds89_register_actions(Blank3DDisplayStack89 *stack,
                                     gverb89_registry *registry)
{
    int i;
    const char *name;
    for (i = 0; i < gwvrb89_action_count(); ++i) {
        name = gwvrb89_action_name(i);
        if (!name || !gverb89_register_action(registry, name,
                                               b3d_ds89_action, stack)) return 0;
    }
    for (i = 0; i < gvvrb89_action_count(); ++i) {
        name = gvvrb89_action_name(i);
        if (!name || !gverb89_register_action(registry, name,
                                               b3d_ds89_action, stack)) return 0;
    }
    for (i = 0; i < gmplyss89_action_count(); ++i) {
        name = gmplyss89_action_name(i);
        if (!name || !gverb89_register_action(registry, name,
                                               b3d_ds89_action, stack)) return 0;
    }
    return 1;
}

static int b3d_ds89_register_conditions(Blank3DDisplayStack89 *stack,
                                        gverb89_registry *registry)
{
    int i;
    const char *name;
    for (i = 0; i < gwvrb89_condition_count(); ++i) {
        name = gwvrb89_condition_name(i);
        if (!name || !gverb89_register_condition(registry, name,
                                                  b3d_ds89_condition, stack)) return 0;
    }
    for (i = 0; i < gvvrb89_condition_count(); ++i) {
        name = gvvrb89_condition_name(i);
        if (!name || !gverb89_register_condition(registry, name,
                                                  b3d_ds89_condition, stack)) return 0;
    }
    for (i = 0; i < gmplyss89_condition_count(); ++i) {
        name = gmplyss89_condition_name(i);
        if (!name || !gverb89_register_condition(registry, name,
                                                  b3d_ds89_condition, stack)) return 0;
    }
    return 1;
}

int blank3d_display_stack89_register_verbs(
    Blank3DDisplayStack89 *stack, gverb89_registry *registry)
{
    if (!stack || !registry) return 0;
    if (!b3d_ds89_register_actions(stack, registry) ||
        !b3d_ds89_register_conditions(stack, registry)) {
        b3d_ds89_status(stack, "GameVerbs registry capacity exhausted");
        return 0;
    }
    b3d_ds89_status(stack, "display sextet verbs registered");
    return 1;
}

const char *blank3d_display_stack89_status(
    const Blank3DDisplayStack89 *stack)
{
    return stack ? stack->status : "display stack unavailable";
}
