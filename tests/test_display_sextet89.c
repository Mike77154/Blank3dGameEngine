#include <stdio.h>
#include <string.h>

#include "blank3d_display_stack89.h"

/* Portable integration test: native window and renderer are fake providers. */
typedef struct TestDisplayHostTag {
    int window_created;
    int window_applied;
    int window_destroyed;
    int window_width;
    int window_height;
    int video_applied;
    int render_width;
    int render_height;
    int output_width;
    int output_height;
    gvc89_rect presentation;
    int gameplay_applied;
    int camera_width;
    int camera_height;
    int scene_width;
    int scene_height;
} TestDisplayHost;

static int test_window_create(void *user, const gwc89_config *config)
{
    TestDisplayHost *host;
    host = (TestDisplayHost *)user;
    if (!host || !config) return 0;
    host->window_created++;
    host->window_width = config->client_width;
    host->window_height = config->client_height;
    return 1;
}

static int test_window_apply(void *user, const gwc89_config *config)
{
    TestDisplayHost *host;
    host = (TestDisplayHost *)user;
    if (!host || !config) return 0;
    host->window_applied++;
    host->window_width = config->client_width;
    host->window_height = config->client_height;
    return 1;
}

static int test_window_destroy(void *user)
{
    TestDisplayHost *host;
    host = (TestDisplayHost *)user;
    if (!host) return 0;
    host->window_destroyed++;
    return 1;
}

static int test_window_query(void *user, int *out_width, int *out_height)
{
    TestDisplayHost *host;
    host = (TestDisplayHost *)user;
    if (!host || !out_width || !out_height) return 0;
    *out_width = host->window_width;
    *out_height = host->window_height;
    return *out_width > 0 && *out_height > 0;
}

static int test_video_apply(void *user, const gvc89_config *config,
                            const gvc89_rect *presentation,
                            int output_width, int output_height)
{
    TestDisplayHost *host;
    host = (TestDisplayHost *)user;
    if (!host || !config || !presentation) return 0;
    host->video_applied++;
    host->render_width = config->resolution_width;
    host->render_height = config->resolution_height;
    host->output_width = output_width;
    host->output_height = output_height;
    host->presentation = *presentation;
    return 1;
}

static int test_gameplay_apply(void *user, const gpss89_config *config)
{
    TestDisplayHost *host;
    host = (TestDisplayHost *)user;
    if (!host || !config) return 0;
    host->gameplay_applied++;
    host->camera_width = config->camera_draw_width;
    host->camera_height = config->camera_draw_height;
    host->scene_width = config->scene_screen_width;
    host->scene_height = config->scene_screen_height;
    return 1;
}

static int run_action(gverb89_registry *registry, const char *name,
                      const char **argv, int argc)
{
    gverb89_call call;
    memset(&call, 0, sizeof(call));
    call.name = name;
    call.argv = argv;
    call.argc = argc;
    return gverb89_perform(registry, &call);
}

static int run_condition(gverb89_registry *registry, const char *name)
{
    gverb89_call call;
    gverb89_result result;
    memset(&call, 0, sizeof(call));
    memset(&result, 0, sizeof(result));
    call.name = name;
    if (gverb89_query(registry, &call, &result) != GVERB89_HANDLED)
        return -1;
    return result.truth;
}

int main(void)
{
    Blank3DDisplayStack89 stack;
    TestDisplayHost host;
    gwc89_provider window_provider;
    gvc89_provider video_provider;
    gpss89_provider gameplay_provider;
    gverb89_registry verbs;
    const char *args[3];
    int expected_actions;
    int expected_conditions;

    memset(&host, 0, sizeof(host));
    memset(&window_provider, 0, sizeof(window_provider));
    memset(&video_provider, 0, sizeof(video_provider));
    memset(&gameplay_provider, 0, sizeof(gameplay_provider));
    blank3d_display_stack89_init(&stack);

    window_provider.user = &host;
    window_provider.create_window = test_window_create;
    window_provider.apply_window = test_window_apply;
    window_provider.destroy_window = test_window_destroy;
    window_provider.query_client_size = test_window_query;
    video_provider.user = &host;
    video_provider.apply_video = test_video_apply;
    gameplay_provider.user = &host;
    gameplay_provider.apply_gameplay_screen = test_gameplay_apply;
    blank3d_display_stack89_set_window_provider(&stack, &window_provider);
    blank3d_display_stack89_set_video_provider(&stack, &video_provider);
    blank3d_display_stack89_set_gameplay_provider(&stack, &gameplay_provider);

    if (!blank3d_display_stack89_load_general_cfg(
            &stack, "config/GeneralConfig.cfg")) return 1;
    if (stack.window.config.client_width != 960 ||
        stack.window.config.client_height != 540) return 2;
    if (stack.video.config.resolution_width != 960 ||
        stack.video.config.resolution_height != 540) return 3;
    if (stack.video.config.scale_mode != GVC89_SCALE_FIT) return 4;

    if (!blank3d_display_stack89_set_gameplay_sizes(
            &stack, 320, 180, 4000, 2000)) return 5;
    if (!blank3d_display_stack89_apply_video_gameplay(&stack)) return 6;
    if (host.render_width != 960 || host.render_height != 540) return 7;
    if (host.camera_width != 320 || host.camera_height != 180 ||
        host.scene_width != 4000 || host.scene_height != 2000) return 8;

    if (!blank3d_display_stack89_create_window(&stack)) return 9;
    if (!host.window_created || !stack.window.created) return 10;
    if (host.output_width != 960 || host.output_height != 540) return 11;

    /* Resize the OS client only: render resolution and gameplay view stay. */
    if (!blank3d_display_stack89_window_resized(&stack, 1280, 720)) return 12;
    if (host.output_width != 1280 || host.output_height != 720) return 13;
    if (host.render_width != 960 || host.render_height != 540) return 14;
    if (host.camera_width != 320 || host.camera_height != 180) return 15;

    gverb89_init(&verbs);
    if (!blank3d_display_stack89_register_verbs(&stack, &verbs)) return 16;
    expected_actions = gwvrb89_action_count() + gvvrb89_action_count() +
                       gmplyss89_action_count();
    expected_conditions = gwvrb89_condition_count() +
                          gvvrb89_condition_count() +
                          gmplyss89_condition_count();
    if (gverb89_count(&verbs, GVERB89_KIND_ACTION) != expected_actions)
        return 17;
    if (gverb89_count(&verbs, GVERB89_KIND_CONDITION) != expected_conditions)
        return 18;

    args[0] = "320"; args[1] = "180";
    if (run_action(&verbs, "video_resolution", args, 2) != GVERB89_HANDLED)
        return 19;
    args[0] = "integer";
    if (run_action(&verbs, "screen_scale", args, 1) != GVERB89_HANDLED)
        return 20;
    if (host.render_width != 320 || host.render_height != 180) return 21;
    if (host.presentation.width != 1280 ||
        host.presentation.height != 720) return 22;
    if (run_condition(&verbs, "video_integer_scale") != 1) return 23;

    args[0] = "640"; args[1] = "360";
    if (run_action(&verbs, "window_size", args, 2) != GVERB89_HANDLED)
        return 24;
    if (host.window_width != 640 || host.window_height != 360) return 25;
    if (host.output_width != 640 || host.output_height != 360) return 32;
    if (host.render_width != 320 || host.render_height != 180) return 33;

    args[0] = "640"; args[1] = "360";
    if (run_action(&verbs, "camera_size_draw", args, 2) != GVERB89_HANDLED)
        return 32;
    args[0] = "6000"; args[1] = "3000";
    if (run_action(&verbs, "scene_screen_size", args, 2) != GVERB89_HANDLED)
        return 33;
    if (host.camera_width != 640 || host.camera_height != 360 ||
        host.scene_width != 6000 || host.scene_height != 3000) return 32;
    if (run_condition(&verbs, "camera_fits_scene") != 1) return 33;
    if (run_condition(&verbs, "window_is_windowed") != 1) return 34;

    args[0] = "0";
    if (run_action(&verbs, "window_decorated", args, 1) != GVERB89_HANDLED) return 35;
    if (run_condition(&verbs, "window_is_decorated") != 0) return 36;
    args[0] = "1";
    if (run_action(&verbs, "window_topmost", args, 1) != GVERB89_HANDLED) return 37;
    if (run_condition(&verbs, "window_is_topmost") != 1) return 38;
    args[0] = "2";
    if (run_action(&verbs, "window_monitor", args, 1) != GVERB89_HANDLED) return 39;
    if (stack.window.config.monitor_index != 2) return 40;

    args[0] = "linear";
    if (run_action(&verbs, "video_filter", args, 1) != GVERB89_HANDLED) return 41;
    if (run_condition(&verbs, "video_filter_linear") != 1) return 42;
    args[0] = "0";
    if (run_action(&verbs, "video_center_output", args, 1) != GVERB89_HANDLED) return 43;
    if (stack.video.config.center_output != 0) return 44;

    args[0] = "0";
    if (run_action(&verbs, "camera_screen_clamp", args, 1) != GVERB89_HANDLED) return 45;
    if (run_condition(&verbs, "camera_clamp_enabled") != 0) return 46;
    args[0] = "1";
    if (run_action(&verbs, "camera_screen_clamp", args, 1) != GVERB89_HANDLED) return 47;
    if (run_condition(&verbs, "camera_clamp_enabled") != 1) return 48;

    if (!gwc89_destroy(&stack.window) || !host.window_destroyed) return 49;

    puts("Blank3D display sextet: native window / render / gameplay spaces PASS");
    return 0;
}
