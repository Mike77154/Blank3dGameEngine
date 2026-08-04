#include <stdio.h>
#include <string.h>
#include "nationalmecanicanimal89.h"

typedef struct test_context_s {
    int transform_samples;
    int socket_samples;
    int pivot_samples;
    int world_samples;
    int visibility_samples;
    int constraint_samples;
    int resolve_calls;
    int apply_calls;
    int event_calls;
    int clock_calls;
    int ease_calls;
    int math_calls;
    int log_calls;
    nm89_geometry_packet last_packet;
} test_context;

static int failures = 0;

static void check_close(const char *label, long actual, long expected,
                        long tolerance)
{
    long difference;
    difference = actual - expected;
    if (difference < 0) {
        difference = -difference;
    }
    if (difference > tolerance) {
        printf("FAIL %-36s actual=%ld expected=%ld tolerance=%ld\n",
               label, actual, expected, tolerance);
        failures += 1;
    } else {
        printf("PASS %s\n", label);
    }
}

static void check_int(const char *label, long actual, long expected)
{
    if (actual != expected) {
        printf("FAIL %-36s actual=%ld expected=%ld\n",
               label, actual, expected);
        failures += 1;
    } else {
        printf("PASS %s\n", label);
    }
}

static int test_sample_transform(
    void *user, const nm89_rig *rig, nm89_i16 part_id,
    nm89_i16 source_id, nm89_transform_sample *out_sample)
{
    test_context *context;
    (void)rig;
    (void)part_id;
    context = (test_context *)user;
    context->transform_samples += 1;
    nm89_transform_sample_identity(out_sample);
    out_sample->transform.move.z = NM89_FX_FROM_INT(source_id);
    out_sample->channels = NM89_CHANNEL_MOVE_Z;
    return NM89_OK;
}

static int test_sample_socket(
    void *user, const nm89_rig *rig, nm89_i16 part_id,
    nm89_i16 socket_id, nm89_transform_sample *out_sample)
{
    test_context *context;
    (void)rig;
    (void)part_id;
    context = (test_context *)user;
    context->socket_samples += 1;
    nm89_transform_sample_identity(out_sample);
    out_sample->transform.move.x = NM89_FX_FROM_INT(socket_id);
    out_sample->channels = NM89_CHANNEL_MOVE_X;
    return NM89_OK;
}

static int test_sample_pivot(
    void *user, const nm89_rig *rig, nm89_i16 part_id,
    nm89_i16 source_id, nm89_vec3 *out_pivot)
{
    test_context *context;
    (void)rig;
    (void)part_id;
    context = (test_context *)user;
    context->pivot_samples += 1;
    nm89_vec3_zero(out_pivot);
    out_pivot->y = NM89_FX_FROM_INT(source_id);
    return NM89_OK;
}

static int test_sample_world(
    void *user, const nm89_rig *rig, nm89_i16 part_id,
    nm89_i16 source_id, nm89_matrix *out_world)
{
    test_context *context;
    (void)rig;
    (void)part_id;
    context = (test_context *)user;
    context->world_samples += 1;
    nm89_matrix_identity(out_world);
    out_world->m[1][3] = NM89_FX_FROM_INT(source_id);
    return NM89_OK;
}

static int test_sample_visibility(
    void *user, const nm89_rig *rig, nm89_i16 part_id,
    nm89_i16 binding_id, nm89_i16 source_id, nm89_u8 *out_visible)
{
    test_context *context;
    (void)rig;
    (void)part_id;
    context = (test_context *)user;
    context->visibility_samples += 1;
    if (binding_id >= 0 && source_id == 99) {
        *out_visible = 0U;
    } else {
        *out_visible = 1U;
    }
    return NM89_OK;
}

static int test_sample_constraint(
    void *user, const nm89_rig *rig, nm89_i16 part_id,
    nm89_i16 source_id, nm89_constraint *out_constraint)
{
    test_context *context;
    (void)rig;
    (void)part_id;
    context = (test_context *)user;
    context->constraint_samples += 1;
    nm89_constraint_free(out_constraint);
    return nm89_constraint_set(out_constraint, NM89_PROP_MOVE_Z,
                               NM89_CONSTRAINT_CLAMPED,
                               NM89_FX_FROM_INT(source_id),
                               0, NM89_FX_ONE);
}

static int test_resolve_geometry(
    void *user, const nm89_rig *rig, nm89_i16 binding_id,
    const nm89_binding *binding,
    nm89_geometry_resolution *out_resolution)
{
    test_context *context;
    (void)rig;
    (void)binding_id;
    (void)binding;
    context = (test_context *)user;
    context->resolve_calls += 1;
    out_resolution->mesh_id = 77;
    return NM89_OK;
}

static void test_apply_geometry(
    void *user, const nm89_rig *rig,
    const nm89_geometry_packet *packet)
{
    test_context *context;
    (void)rig;
    context = (test_context *)user;
    context->apply_calls += 1;
    context->last_packet = *packet;
}

static void test_emit_event(
    void *user, const nm89_rig *rig,
    const nm89_clip_event *event_value)
{
    test_context *context;
    (void)rig;
    (void)event_value;
    context = (test_context *)user;
    context->event_calls += 1;
}

static nm89_u16 test_clock(void *user, const nm89_rig *rig)
{
    test_context *context;
    (void)rig;
    context = (test_context *)user;
    context->clock_calls += 1;
    return 2U;
}

static void test_log(void *user, int level, int code,
                     const char *message)
{
    test_context *context;
    (void)level;
    (void)code;
    (void)message;
    context = (test_context *)user;
    context->log_calls += 1;
}

static void test_matrix_identity(void *user, nm89_matrix *out_matrix)
{
    test_context *context;
    context = (test_context *)user;
    context->math_calls += 1;
    nm89_matrix_identity(out_matrix);
}

static void test_matrix_multiply(void *user, nm89_matrix *out_matrix,
                                 const nm89_matrix *a,
                                 const nm89_matrix *b)
{
    test_context *context;
    context = (test_context *)user;
    context->math_calls += 1;
    nm89_matrix_multiply(out_matrix, a, b);
}

static void test_matrix_from_transform(
    void *user, nm89_matrix *out_matrix,
    const nm89_transform *transform_value, const nm89_vec3 *pivot)
{
    test_context *context;
    context = (test_context *)user;
    context->math_calls += 1;
    nm89_matrix_from_transform(out_matrix, transform_value, pivot);
}

static void test_matrix_transform_point(
    void *user, const nm89_matrix *matrix,
    nm89_fx x, nm89_fx y, nm89_fx z,
    nm89_fx *out_x, nm89_fx *out_y, nm89_fx *out_z)
{
    test_context *context;
    context = (test_context *)user;
    context->math_calls += 1;
    nm89_matrix_transform_point(matrix, x, y, z,
                                out_x, out_y, out_z);
}

static nm89_fx test_ease(void *user, nm89_u8 interpolation,
                         nm89_fx normalized_time)
{
    test_context *context;
    (void)interpolation;
    context = (test_context *)user;
    context->ease_calls += 1;
    return normalized_time;
}

static void provider_identity(nm89_provider *provider)
{
    memset(provider, 0, sizeof(*provider));
}

static void test_fixed_math(void)
{
    nm89_fx value;
    value = nm89_mul(NM89_FX_FROM_INT(3),
                     nm89_fx_from_ratio(1, 2));
    check_int("Q16.16 multiply", value,
              NM89_FX_FROM_INT(1) + NM89_FX_HALF);
    check_int("sin 90 degrees", nm89_sin_deg(NM89_FX_FROM_INT(90)),
              NM89_FX_ONE);
    check_int("cos 180 degrees", nm89_cos_deg(NM89_FX_FROM_INT(180)),
              -NM89_FX_ONE);
}

static void test_full_pipeline(void)
{
    nm89_rig rig;
    nm89_provider provider;
    test_context context;
    nm89_transform home;
    nm89_constraint static_constraint;
    nm89_pivot pivot;
    nm89_alignment alignment;
    nm89_clip_event event_value;
    nm89_i16 provider_id;
    nm89_i16 constraint_id;
    nm89_i16 pivot_id;
    nm89_i16 alignment_id;
    nm89_i16 root;
    nm89_i16 slide;
    nm89_i16 binding;
    nm89_i16 clip;
    nm89_i16 track;
    const nm89_pose *pose;
    const nm89_matrix *world;
    nm89_fx point_x;
    nm89_fx point_y;
    nm89_fx point_z;
    int result;

    memset(&context, 0, sizeof(context));
    provider_identity(&provider);
    provider.user = &context;
    provider.sample_transform = test_sample_transform;
    provider.sample_socket = test_sample_socket;
    provider.sample_pivot = test_sample_pivot;
    provider.sample_world = test_sample_world;
    provider.sample_visibility = test_sample_visibility;
    provider.sample_constraint = test_sample_constraint;
    provider.resolve_geometry = test_resolve_geometry;
    provider.apply_geometry = test_apply_geometry;
    provider.emit_event = test_emit_event;
    provider.sample_delta_ticks = test_clock;
    provider.log = test_log;
    provider.matrix_identity = test_matrix_identity;
    provider.matrix_multiply = test_matrix_multiply;
    provider.matrix_from_transform = test_matrix_from_transform;
    provider.matrix_transform_point = test_matrix_transform_point;
    provider.ease = test_ease;

    nm89_rig_init(&rig, 123);
    result = nm89_provider_add(&rig, &provider, &provider_id);
    check_int("provider registry", result, NM89_OK);
    check_int("math provider selector",
              nm89_rig_set_math_provider(&rig, provider_id), NM89_OK);
    check_int("easing provider selector",
              nm89_rig_set_easing_provider(&rig, provider_id), NM89_OK);
    check_int("clock provider selector",
              nm89_rig_set_clock_provider(&rig, provider_id), NM89_OK);

    nm89_transform_identity(&home);
    nm89_constraint_lock_to_transform(&static_constraint, &home);
    check_int("static slide constraint",
              nm89_constraint_set(&static_constraint,
                                  NM89_PROP_MOVE_Z,
                                  NM89_CONSTRAINT_CLAMPED,
                                  NM89_FX_FROM_INT(-4), 0,
                                  NM89_FX_ONE), NM89_OK);
    check_int("constraint bank",
              nm89_constraint_add(&rig, &static_constraint,
                                  &constraint_id), NM89_OK);

    nm89_pivot_identity(&pivot);
    pivot.point.y = NM89_FX_FROM_INT(1);
    check_int("pivot bank", nm89_pivot_add(&rig, &pivot, &pivot_id),
              NM89_OK);

    nm89_alignment_identity(&alignment);
    alignment.transform.move.x = NM89_FX_FROM_INT(2);
    check_int("alignment bank",
              nm89_alignment_add(&rig, &alignment, &alignment_id),
              NM89_OK);

    home.move.x = NM89_FX_FROM_INT(1);
    check_int("root part",
              nm89_part_add(&rig, "root", NM89_ROOT_PART, 10,
                            &home, NM89_INVALID_ID, NM89_INVALID_ID,
                            1U, &root), NM89_OK);
    nm89_transform_identity(&home);
    check_int("slide part",
              nm89_part_add(&rig, "slide", root, 20, &home,
                            constraint_id, pivot_id, 1U, &slide),
              NM89_OK);

    check_int("transform provider binding",
              nm89_part_bind_transform_provider(
                  &rig, slide, provider_id, -10,
                  NM89_CHANNEL_MOVE_Z, NM89_PROVIDER_REPLACE),
              NM89_OK);
    check_int("socket provider binding",
              nm89_part_bind_socket_provider(
                  &rig, slide, provider_id, 10,
                  NM89_SOCKET_PARENT_SPACE), NM89_OK);
    check_int("pivot provider binding",
              nm89_part_bind_pivot_provider(
                  &rig, slide, provider_id, 2), NM89_OK);
    check_int("world provider binding",
              nm89_part_bind_world_provider(
                  &rig, slide, provider_id, 20,
                  NM89_WORLD_PREMULTIPLY), NM89_OK);
    check_int("visibility provider binding",
              nm89_part_bind_visibility_provider(
                  &rig, slide, provider_id, 99,
                  NM89_VISIBILITY_AND), NM89_OK);
    check_int("constraint provider binding",
              nm89_part_bind_constraint_provider(
                  &rig, slide, provider_id, -2,
                  NM89_DYNAMIC_CONSTRAINT_INTERSECT), NM89_OK);

    check_int("geometry binding",
              nm89_binding_add(&rig, slide, 5,
                               NM89_SELECTOR_GROUP, 3, 11,
                               alignment_id, provider_id, 55,
                               &binding), NM89_OK);

    check_int("clip begin",
              nm89_clip_begin(&rig, 88, 10U, 0U, &clip), NM89_OK);
    check_int("track begin",
              nm89_clip_add_track(&rig, root, NM89_PROP_MOVE_Y,
                                  NM89_INTERP_CUSTOM, &track), NM89_OK);
    check_int("track key zero",
              nm89_track_add_key(&rig, track, 0U, 0), NM89_OK);
    check_int("track key ten",
              nm89_track_add_key(&rig, track, 10U,
                                 NM89_FX_FROM_INT(10)), NM89_OK);
    event_value.tick = 2U;
    event_value.part_id = slide;
    event_value.code = 700;
    event_value.value = 9;
    event_value.type = NM89_EVENT_SOUND;
    check_int("clip event",
              nm89_clip_add_event(&rig, &event_value), NM89_OK);
    check_int("clip end", nm89_clip_end(&rig), NM89_OK);
    check_int("action binding",
              nm89_action_bind(&rig, 1, clip, NM89_ACTION_RESTART),
              NM89_OK);
    check_int("action trigger", nm89_trigger_action(&rig, 1), NM89_OK);

    check_int("clock update", nm89_update_from_clock(&rig), NM89_OK);
    pose = nm89_part_get_pose(&rig, slide);
    check_int("provider then static/dynamic constraints",
              pose->resolved.move.z, NM89_FX_FROM_INT(-2));
    pose = nm89_part_get_pose(&rig, root);
    check_close("custom eased clip channel",
                pose->resolved.move.y, NM89_FX_FROM_INT(2), 4);
    check_int("event crossed at tick two", context.event_calls, 1);

    world = nm89_part_get_world(&rig, slide);
    check_int("parent + parent-space socket X",
              world->m[0][3], NM89_FX_FROM_INT(11));
    check_close("world provider premultiply Y",
                world->m[1][3], NM89_FX_FROM_INT(22), 4);
    check_int("resolved provider Z",
              world->m[2][3], NM89_FX_FROM_INT(-2));

    check_int("flush", nm89_flush(&rig), NM89_OK);
    check_int("resolver called", context.resolve_calls, 1);
    check_int("output called", context.apply_calls, 1);
    check_int("resolver changed mesh", context.last_packet.mesh_id, 77);
    check_int("binding visibility provider", context.last_packet.visible, 0);
    check_int("alignment applied to world X",
              context.last_packet.world.m[0][3], NM89_FX_FROM_INT(13));

    point_x = 0;
    point_y = 0;
    point_z = 0;
    nm89_rig_transform_point(&rig, &context.last_packet.world,
                             0, 0, 0,
                             &point_x, &point_y, &point_z);
    check_int("provider-aware point transform X",
              point_x, NM89_FX_FROM_INT(13));
    check_close("provider-aware point transform Y",
                point_y, NM89_FX_FROM_INT(22), 4);
    check_int("provider-aware point transform Z",
              point_z, NM89_FX_FROM_INT(-2));

    check_int("transform samples", context.transform_samples, 1);
    check_int("socket samples", context.socket_samples, 1);
    check_int("pivot samples", context.pivot_samples, 1);
    check_int("world samples", context.world_samples, 1);
    check_int("constraint samples", context.constraint_samples, 1);
    check_int("clock samples", context.clock_calls, 1);
    check_int("custom easing samples", context.ease_calls, 1);
    check_int("math provider used", context.math_calls > 0, 1);
    check_int("part lookup", nm89_part_find(&rig, "slide"), slide);
    check_int("binding id stable", binding, 0);
}

static void test_constraints(void)
{
    nm89_constraint constraint_value;
    nm89_transform home;
    nm89_rig rig;
    nm89_i16 constraint_id;
    nm89_i16 part_id;
    const nm89_pose *pose;

    nm89_transform_identity(&home);
    nm89_constraint_free(&constraint_value);
    nm89_constraint_set(&constraint_value, NM89_PROP_ROTATE_Y,
                        NM89_CONSTRAINT_WRAPPED,
                        0, NM89_FX_FROM_INT(360), NM89_FX_ONE);
    nm89_constraint_set(&constraint_value, NM89_PROP_MOVE_X,
                        NM89_CONSTRAINT_STEPPED,
                        0, NM89_FX_FROM_INT(10),
                        NM89_FX_FROM_INT(2));
    nm89_rig_init(&rig, 1);
    nm89_constraint_add(&rig, &constraint_value, &constraint_id);
    nm89_part_add(&rig, "test", NM89_ROOT_PART, 0, &home,
                  constraint_id, NM89_INVALID_ID, 1U, &part_id);
    nm89_part_set_channel(&rig, part_id, NM89_PROP_ROTATE_Y,
                          NM89_FX_FROM_INT(450));
    nm89_part_set_channel(&rig, part_id, NM89_PROP_MOVE_X,
                          NM89_FX_FROM_INT(7));
    nm89_evaluate(&rig);
    pose = nm89_part_get_pose(&rig, part_id);
    check_int("wrapped rotation", pose->resolved.rotate.y,
              NM89_FX_FROM_INT(90));
    check_int("stepped translation", pose->resolved.move.x,
              NM89_FX_FROM_INT(6));
}

int main(void)
{
    test_fixed_math();
    test_constraints();
    test_full_pipeline();
    printf("\nsizeof(nm89_part)=%lu\n", (unsigned long)sizeof(nm89_part));
    printf("sizeof(nm89_rig)=%lu\n", (unsigned long)sizeof(nm89_rig));
    if (failures != 0) {
        printf("\n%d test(s) failed.\n", failures);
        return 1;
    }
    printf("\nAll NationalMecanicanimal89 tests passed.\n");
    return 0;
}
