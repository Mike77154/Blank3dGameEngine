#ifndef NATIONALMECANICANIMAL89_H
#define NATIONALMECANICANIMAL89_H

#ifdef __cplusplus
extern "C" {
#endif

/*
    NationalMecanicanimal89
    Rigid mechanical animation and provider bridge for strict C89.
    Static storage, Q16.16 fixed point, no heap, integer-only arithmetic.
*/

#define NM89_VERSION_MAJOR 0
#define NM89_VERSION_MINOR 1
#define NM89_VERSION_PATCH 0

#ifndef NM89_MAX_PARTS
#define NM89_MAX_PARTS 64
#endif
#ifndef NM89_MAX_CONSTRAINTS
#define NM89_MAX_CONSTRAINTS 32
#endif
#ifndef NM89_MAX_PIVOTS
#define NM89_MAX_PIVOTS 32
#endif
#ifndef NM89_MAX_ALIGNMENTS
#define NM89_MAX_ALIGNMENTS 64
#endif
#ifndef NM89_MAX_BINDINGS
#define NM89_MAX_BINDINGS 128
#endif
#ifndef NM89_MAX_CLIPS
#define NM89_MAX_CLIPS 16
#endif
#ifndef NM89_MAX_TRACKS
#define NM89_MAX_TRACKS 128
#endif
#ifndef NM89_MAX_KEYS
#define NM89_MAX_KEYS 512
#endif
#ifndef NM89_MAX_EVENTS
#define NM89_MAX_EVENTS 128
#endif
#ifndef NM89_MAX_ACTIONS
#define NM89_MAX_ACTIONS 32
#endif
#ifndef NM89_MAX_PROVIDERS
#define NM89_MAX_PROVIDERS 8
#endif
#ifndef NM89_NAME_CAPACITY
#define NM89_NAME_CAPACITY 24
#endif

#define NM89_OK 0
#define NM89_ERR_ARGUMENT (-1)
#define NM89_ERR_CAPACITY (-2)
#define NM89_ERR_RANGE (-3)
#define NM89_ERR_STATE (-4)
#define NM89_ERR_HIERARCHY (-5)
#define NM89_ERR_PROVIDER (-6)
#define NM89_ERR_NOT_FOUND (-7)

#define NM89_INVALID_ID (-1)
#define NM89_ROOT_PART (-1)
#define NM89_WHOLE_RESOURCE (-1)

#define NM89_FX_SHIFT 16
#define NM89_FX_ONE 65536
#define NM89_FX_HALF 32768
#define NM89_FX_FROM_INT(v) ((nm89_fx)((v) * NM89_FX_ONE))
#define NM89_FX_TO_INT(v) ((nm89_i32)((v) / NM89_FX_ONE))

#define NM89_AXIS_X 0
#define NM89_AXIS_Y 1
#define NM89_AXIS_Z 2

#define NM89_PROP_MOVE_X 0
#define NM89_PROP_MOVE_Y 1
#define NM89_PROP_MOVE_Z 2
#define NM89_PROP_ROTATE_X 3
#define NM89_PROP_ROTATE_Y 4
#define NM89_PROP_ROTATE_Z 5
#define NM89_PROP_SCALE_X 6
#define NM89_PROP_SCALE_Y 7
#define NM89_PROP_SCALE_Z 8
#define NM89_PROP_VISIBLE 9
#define NM89_PROPERTY_COUNT 10

#define NM89_CHANNEL_MOVE_X (1UL << NM89_PROP_MOVE_X)
#define NM89_CHANNEL_MOVE_Y (1UL << NM89_PROP_MOVE_Y)
#define NM89_CHANNEL_MOVE_Z (1UL << NM89_PROP_MOVE_Z)
#define NM89_CHANNEL_ROTATE_X (1UL << NM89_PROP_ROTATE_X)
#define NM89_CHANNEL_ROTATE_Y (1UL << NM89_PROP_ROTATE_Y)
#define NM89_CHANNEL_ROTATE_Z (1UL << NM89_PROP_ROTATE_Z)
#define NM89_CHANNEL_SCALE_X (1UL << NM89_PROP_SCALE_X)
#define NM89_CHANNEL_SCALE_Y (1UL << NM89_PROP_SCALE_Y)
#define NM89_CHANNEL_SCALE_Z (1UL << NM89_PROP_SCALE_Z)
#define NM89_CHANNEL_VISIBLE (1UL << NM89_PROP_VISIBLE)
#define NM89_CHANNEL_POSITION (NM89_CHANNEL_MOVE_X | NM89_CHANNEL_MOVE_Y | NM89_CHANNEL_MOVE_Z)
#define NM89_CHANNEL_ROTATION (NM89_CHANNEL_ROTATE_X | NM89_CHANNEL_ROTATE_Y | NM89_CHANNEL_ROTATE_Z)
#define NM89_CHANNEL_SCALE (NM89_CHANNEL_SCALE_X | NM89_CHANNEL_SCALE_Y | NM89_CHANNEL_SCALE_Z)
#define NM89_CHANNEL_TRANSFORM (NM89_CHANNEL_POSITION | NM89_CHANNEL_ROTATION | NM89_CHANNEL_SCALE)
#define NM89_CHANNEL_ALL (NM89_CHANNEL_TRANSFORM | NM89_CHANNEL_VISIBLE)

#define NM89_CONSTRAINT_LOCKED 0
#define NM89_CONSTRAINT_FREE 1
#define NM89_CONSTRAINT_CLAMPED 2
#define NM89_CONSTRAINT_WRAPPED 3
#define NM89_CONSTRAINT_STEPPED 4

#define NM89_PROVIDER_REPLACE 0
#define NM89_PROVIDER_ADDITIVE 1

#define NM89_SOCKET_PARENT_SPACE 0
#define NM89_SOCKET_WORLD_SPACE 1

#define NM89_WORLD_PREMULTIPLY 0
#define NM89_WORLD_REPLACE 1

#define NM89_VISIBILITY_AND 0
#define NM89_VISIBILITY_REPLACE 1

#define NM89_DYNAMIC_CONSTRAINT_INTERSECT 0
#define NM89_DYNAMIC_CONSTRAINT_REPLACE 1

#define NM89_SELECTOR_WHOLE 0
#define NM89_SELECTOR_OBJECT 1
#define NM89_SELECTOR_GROUP 2
#define NM89_SELECTOR_SUBMESH 3
#define NM89_SELECTOR_ENGINE_MESH 4
#define NM89_SELECTOR_PROCEDURAL 5

#define NM89_INTERP_STEP 0
#define NM89_INTERP_LINEAR 1
#define NM89_INTERP_SMOOTH 2
#define NM89_INTERP_EASE_IN 3
#define NM89_INTERP_EASE_OUT 4
#define NM89_INTERP_CUSTOM 5

#define NM89_ACTION_RESTART 0
#define NM89_ACTION_IGNORE_IF_PLAYING 1
#define NM89_ACTION_REVERSE 2
#define NM89_ACTION_QUEUE 3

#define NM89_EVENT_MARKER 0
#define NM89_EVENT_VISIBILITY 1
#define NM89_EVENT_SOUND 2
#define NM89_EVENT_USER 3

#define NM89_PART_ENABLED 1U
#define NM89_PART_VISIBLE 2U
#define NM89_BINDING_VISIBLE 1U
#define NM89_CLIP_LOOP 1U
#define NM89_CLIP_ENABLED 2U

#define NM89_LOG_INFO 0
#define NM89_LOG_WARNING 1
#define NM89_LOG_ERROR 2

/* C89 compile-time width checks for the intended Win32/MSYS2 target. */
typedef char nm89_int_must_be_32_bits[(sizeof(int) == 4) ? 1 : -1];
typedef char nm89_short_must_be_16_bits[(sizeof(short) == 2) ? 1 : -1];

typedef signed int nm89_i32;
typedef unsigned int nm89_u32;
typedef signed short nm89_i16;
typedef unsigned short nm89_u16;
typedef signed char nm89_i8;
typedef unsigned char nm89_u8;
typedef nm89_i32 nm89_fx;

typedef struct nm89_vec3_s {
    nm89_fx x;
    nm89_fx y;
    nm89_fx z;
} nm89_vec3;

typedef struct nm89_transform_s {
    nm89_vec3 move;
    nm89_vec3 rotate;
    nm89_vec3 scale;
} nm89_transform;

typedef struct nm89_matrix_s {
    nm89_fx m[4][4];
} nm89_matrix;

typedef struct nm89_transform_sample_s {
    nm89_transform transform;
    unsigned long channels;
    nm89_u8 visible;
    nm89_u8 has_visibility;
} nm89_transform_sample;

typedef struct nm89_axis_constraint_s {
    nm89_fx minimum;
    nm89_fx maximum;
    nm89_fx step;
    nm89_u8 mode;
} nm89_axis_constraint;

typedef struct nm89_constraint_s {
    nm89_axis_constraint move[3];
    nm89_axis_constraint rotate[3];
    nm89_axis_constraint scale[3];
} nm89_constraint;

typedef struct nm89_pivot_s {
    nm89_vec3 point;
} nm89_pivot;

typedef struct nm89_pose_s {
    nm89_transform requested;
    nm89_transform resolved;
    nm89_u8 requested_visible;
    nm89_u8 resolved_visible;
} nm89_pose;

typedef struct nm89_part_s {
    nm89_i16 parent_part;
    nm89_i16 constraint_id;
    nm89_i16 pivot_id;
    nm89_i16 user_tag;
    nm89_u16 flags;
    nm89_u8 world_visible;
    char name[NM89_NAME_CAPACITY];

    nm89_transform home;
    nm89_transform manual;
    nm89_pose pose;
    nm89_matrix world;

    nm89_i16 transform_provider;
    nm89_i16 transform_source;
    unsigned long transform_channels;
    nm89_u8 transform_mode;

    nm89_i16 socket_provider;
    nm89_i16 socket_source;
    nm89_u8 socket_space;

    nm89_i16 pivot_provider;
    nm89_i16 pivot_source;

    nm89_i16 world_provider;
    nm89_i16 world_source;
    nm89_u8 world_mode;

    nm89_i16 visibility_provider;
    nm89_i16 visibility_source;
    nm89_u8 visibility_mode;

    nm89_i16 constraint_provider;
    nm89_i16 constraint_source;
    nm89_u8 dynamic_constraint_mode;
} nm89_part;

typedef struct nm89_alignment_s {
    nm89_transform transform;
} nm89_alignment;

typedef struct nm89_binding_s {
    nm89_i16 owner_part;
    nm89_i16 resource_id;
    nm89_i16 selector_id;
    nm89_i16 mesh_id;
    nm89_i16 alignment_id;
    nm89_i16 resolver_provider;
    nm89_i16 user_tag;
    nm89_u8 selector_type;
    nm89_u8 flags;
} nm89_binding;

typedef struct nm89_geometry_resolution_s {
    nm89_i16 resource_id;
    nm89_i16 selector_id;
    nm89_i16 mesh_id;
    nm89_i16 alignment_id;
    nm89_u8 selector_type;
    nm89_u8 visible;
} nm89_geometry_resolution;

typedef struct nm89_geometry_packet_s {
    nm89_i16 binding_id;
    nm89_i16 part_id;
    nm89_i16 parent_part;
    nm89_i16 part_tag;
    nm89_i16 binding_tag;
    nm89_i16 resource_id;
    nm89_i16 selector_id;
    nm89_i16 mesh_id;
    nm89_u8 selector_type;
    nm89_u8 visible;
    nm89_transform local_resolved;
    nm89_matrix world;
} nm89_geometry_packet;

typedef struct nm89_key_s {
    nm89_u16 tick;
    nm89_fx value;
} nm89_key;

typedef struct nm89_track_s {
    nm89_i16 part_id;
    nm89_u16 first_key;
    nm89_u16 key_count;
    nm89_u8 property;
    nm89_u8 interpolation;
} nm89_track;

typedef struct nm89_clip_event_s {
    nm89_u16 tick;
    nm89_i16 part_id;
    nm89_i16 code;
    nm89_i32 value;
    nm89_u8 type;
} nm89_clip_event;

typedef struct nm89_clip_s {
    nm89_u16 first_track;
    nm89_u16 track_count;
    nm89_u16 first_event;
    nm89_u16 event_count;
    nm89_u16 length_ticks;
    nm89_i16 user_tag;
    nm89_u8 flags;
} nm89_clip;

typedef struct nm89_action_s {
    nm89_i16 action_id;
    nm89_i16 clip_id;
    nm89_u8 policy;
    nm89_u8 enabled;
} nm89_action;

typedef struct nm89_player_s {
    nm89_i16 clip_id;
    nm89_i16 queued_clip;
    nm89_u16 tick;
    nm89_u16 previous_tick;
    nm89_u8 playing;
    nm89_u8 reverse;
} nm89_player;

struct nm89_rig_s;

/* Provider callbacks. Any callback may be NULL. */
typedef int (*nm89_sample_transform_fn)(
    void *user, const struct nm89_rig_s *rig, nm89_i16 part_id,
    nm89_i16 source_id, nm89_transform_sample *out_sample);

typedef int (*nm89_sample_socket_fn)(
    void *user, const struct nm89_rig_s *rig, nm89_i16 part_id,
    nm89_i16 socket_id, nm89_transform_sample *out_sample);

typedef int (*nm89_sample_pivot_fn)(
    void *user, const struct nm89_rig_s *rig, nm89_i16 part_id,
    nm89_i16 source_id, nm89_vec3 *out_pivot);

typedef int (*nm89_sample_world_fn)(
    void *user, const struct nm89_rig_s *rig, nm89_i16 part_id,
    nm89_i16 source_id, nm89_matrix *out_world);

typedef int (*nm89_sample_visibility_fn)(
    void *user, const struct nm89_rig_s *rig, nm89_i16 part_id,
    nm89_i16 binding_id, nm89_i16 source_id, nm89_u8 *out_visible);

typedef int (*nm89_sample_constraint_fn)(
    void *user, const struct nm89_rig_s *rig, nm89_i16 part_id,
    nm89_i16 source_id, nm89_constraint *out_constraint);

typedef int (*nm89_resolve_geometry_fn)(
    void *user, const struct nm89_rig_s *rig, nm89_i16 binding_id,
    const nm89_binding *binding, nm89_geometry_resolution *out_resolution);

typedef void (*nm89_apply_geometry_fn)(
    void *user, const struct nm89_rig_s *rig,
    const nm89_geometry_packet *packet);

typedef void (*nm89_emit_event_fn)(
    void *user, const struct nm89_rig_s *rig,
    const nm89_clip_event *event_value);

typedef nm89_u16 (*nm89_sample_delta_ticks_fn)(
    void *user, const struct nm89_rig_s *rig);

typedef void (*nm89_log_fn)(
    void *user, int level, int code, const char *message);

typedef void (*nm89_matrix_identity_fn)(
    void *user, nm89_matrix *out_matrix);

typedef void (*nm89_matrix_multiply_fn)(
    void *user, nm89_matrix *out_matrix,
    const nm89_matrix *a, const nm89_matrix *b);

typedef void (*nm89_matrix_from_transform_fn)(
    void *user, nm89_matrix *out_matrix,
    const nm89_transform *transform_value, const nm89_vec3 *pivot);

typedef void (*nm89_matrix_transform_point_fn)(
    void *user, const nm89_matrix *matrix,
    nm89_fx x, nm89_fx y, nm89_fx z,
    nm89_fx *out_x, nm89_fx *out_y, nm89_fx *out_z);

typedef nm89_fx (*nm89_ease_fn)(
    void *user, nm89_u8 interpolation, nm89_fx normalized_time);

typedef struct nm89_provider_s {
    void *user;
    nm89_sample_transform_fn sample_transform;
    nm89_sample_socket_fn sample_socket;
    nm89_sample_pivot_fn sample_pivot;
    nm89_sample_world_fn sample_world;
    nm89_sample_visibility_fn sample_visibility;
    nm89_sample_constraint_fn sample_constraint;
    nm89_resolve_geometry_fn resolve_geometry;
    nm89_apply_geometry_fn apply_geometry;
    nm89_emit_event_fn emit_event;
    nm89_sample_delta_ticks_fn sample_delta_ticks;
    nm89_log_fn log;
    nm89_matrix_identity_fn matrix_identity;
    nm89_matrix_multiply_fn matrix_multiply;
    nm89_matrix_from_transform_fn matrix_from_transform;
    nm89_matrix_transform_point_fn matrix_transform_point;
    nm89_ease_fn ease;
} nm89_provider;

typedef struct nm89_rig_s {
    nm89_i16 rig_tag;
    nm89_u16 part_count;
    nm89_u16 constraint_count;
    nm89_u16 pivot_count;
    nm89_u16 alignment_count;
    nm89_u16 binding_count;
    nm89_u16 clip_count;
    nm89_u16 track_count;
    nm89_u16 key_count;
    nm89_u16 event_count;
    nm89_u16 action_count;
    nm89_u16 provider_count;

    nm89_i16 builder_clip;
    nm89_i16 builder_track;
    nm89_i16 math_provider;
    nm89_i16 easing_provider;
    nm89_i16 clock_provider;

    nm89_part parts[NM89_MAX_PARTS];
    nm89_constraint constraints[NM89_MAX_CONSTRAINTS];
    nm89_pivot pivots[NM89_MAX_PIVOTS];
    nm89_alignment alignments[NM89_MAX_ALIGNMENTS];
    nm89_binding bindings[NM89_MAX_BINDINGS];
    nm89_clip clips[NM89_MAX_CLIPS];
    nm89_track tracks[NM89_MAX_TRACKS];
    nm89_key keys[NM89_MAX_KEYS];
    nm89_clip_event events[NM89_MAX_EVENTS];
    nm89_action actions[NM89_MAX_ACTIONS];
    nm89_provider providers[NM89_MAX_PROVIDERS];
    nm89_player player;
} nm89_rig;

/* Fixed-point and math. */
nm89_fx nm89_fx_from_int(nm89_i32 value);
nm89_i32 nm89_fx_to_int(nm89_fx value);
nm89_fx nm89_fx_from_ratio(nm89_i32 numerator, nm89_i32 denominator);
nm89_fx nm89_mul(nm89_fx a, nm89_fx b);
nm89_fx nm89_div(nm89_fx a, nm89_fx b);
nm89_fx nm89_lerp(nm89_fx a, nm89_fx b, nm89_fx t);
nm89_fx nm89_sin_deg(nm89_fx degrees);
nm89_fx nm89_cos_deg(nm89_fx degrees);

void nm89_vec3_zero(nm89_vec3 *value);
void nm89_transform_identity(nm89_transform *value);
void nm89_transform_sample_identity(nm89_transform_sample *sample);
void nm89_matrix_identity(nm89_matrix *matrix);
void nm89_matrix_multiply(nm89_matrix *out_matrix,
                          const nm89_matrix *a,
                          const nm89_matrix *b);
void nm89_matrix_from_transform(nm89_matrix *out_matrix,
                                const nm89_transform *transform_value,
                                const nm89_vec3 *pivot);
void nm89_matrix_transform_point(const nm89_matrix *matrix,
                                 nm89_fx x, nm89_fx y, nm89_fx z,
                                 nm89_fx *out_x,
                                 nm89_fx *out_y,
                                 nm89_fx *out_z);

/* Definitions and reusable banks. */
void nm89_constraint_free(nm89_constraint *constraint_value);
void nm89_constraint_lock_to_transform(nm89_constraint *constraint_value,
                                       const nm89_transform *transform_value);
int nm89_constraint_set(nm89_constraint *constraint_value,
                        int property, int mode,
                        nm89_fx minimum, nm89_fx maximum,
                        nm89_fx step);
void nm89_pivot_identity(nm89_pivot *pivot);
void nm89_alignment_identity(nm89_alignment *alignment);

/* Rig and provider registry. */
void nm89_rig_init(nm89_rig *rig, nm89_i16 rig_tag);
int nm89_provider_add(nm89_rig *rig, const nm89_provider *provider,
                      nm89_i16 *out_provider_id);
int nm89_rig_set_math_provider(nm89_rig *rig, nm89_i16 provider_id);
int nm89_rig_set_easing_provider(nm89_rig *rig, nm89_i16 provider_id);
int nm89_rig_set_clock_provider(nm89_rig *rig, nm89_i16 provider_id);

int nm89_constraint_add(nm89_rig *rig,
                        const nm89_constraint *constraint_value,
                        nm89_i16 *out_constraint_id);
int nm89_pivot_add(nm89_rig *rig, const nm89_pivot *pivot,
                   nm89_i16 *out_pivot_id);
int nm89_alignment_add(nm89_rig *rig, const nm89_alignment *alignment,
                       nm89_i16 *out_alignment_id);

/* Parts and provider bindings. */
int nm89_part_add(nm89_rig *rig, const char *name,
                  nm89_i16 parent_part, nm89_i16 user_tag,
                  const nm89_transform *home,
                  nm89_i16 constraint_id, nm89_i16 pivot_id,
                  nm89_u8 visible, nm89_i16 *out_part_id);
int nm89_part_find(const nm89_rig *rig, const char *name);
int nm89_part_set_parent(nm89_rig *rig, nm89_i16 part_id,
                         nm89_i16 parent_part);
int nm89_part_set_manual_transform(nm89_rig *rig, nm89_i16 part_id,
                                   const nm89_transform *transform_value);
int nm89_part_set_channel(nm89_rig *rig, nm89_i16 part_id,
                          int property, nm89_fx value);
int nm89_part_set_visible(nm89_rig *rig, nm89_i16 part_id,
                          nm89_u8 visible);
int nm89_part_set_enabled(nm89_rig *rig, nm89_i16 part_id,
                          nm89_u8 enabled);
void nm89_rig_reset_manual_pose(nm89_rig *rig);

int nm89_part_bind_transform_provider(nm89_rig *rig, nm89_i16 part_id,
                                      nm89_i16 provider_id,
                                      nm89_i16 source_id,
                                      unsigned long channels,
                                      nm89_u8 mode);
int nm89_part_bind_socket_provider(nm89_rig *rig, nm89_i16 part_id,
                                   nm89_i16 provider_id,
                                   nm89_i16 socket_id,
                                   nm89_u8 socket_space);
int nm89_part_bind_pivot_provider(nm89_rig *rig, nm89_i16 part_id,
                                  nm89_i16 provider_id,
                                  nm89_i16 source_id);
int nm89_part_bind_world_provider(nm89_rig *rig, nm89_i16 part_id,
                                  nm89_i16 provider_id,
                                  nm89_i16 source_id,
                                  nm89_u8 world_mode);
int nm89_part_bind_visibility_provider(nm89_rig *rig, nm89_i16 part_id,
                                       nm89_i16 provider_id,
                                       nm89_i16 source_id,
                                       nm89_u8 visibility_mode);
int nm89_part_bind_constraint_provider(nm89_rig *rig, nm89_i16 part_id,
                                       nm89_i16 provider_id,
                                       nm89_i16 source_id,
                                       nm89_u8 constraint_mode);
int nm89_part_clear_provider_bindings(nm89_rig *rig, nm89_i16 part_id);

/* Geometry bindings. */
int nm89_binding_add(nm89_rig *rig, nm89_i16 owner_part,
                     nm89_i16 resource_id, nm89_u8 selector_type,
                     nm89_i16 selector_id, nm89_i16 mesh_id,
                     nm89_i16 alignment_id,
                     nm89_i16 resolver_provider,
                     nm89_i16 user_tag,
                     nm89_i16 *out_binding_id);
int nm89_binding_set_visible(nm89_rig *rig, nm89_i16 binding_id,
                             nm89_u8 visible);
int nm89_binding_set_resolver(nm89_rig *rig, nm89_i16 binding_id,
                              nm89_i16 provider_id);

/* Clip builder, player, actions and events. */
int nm89_clip_begin(nm89_rig *rig, nm89_i16 user_tag,
                    nm89_u16 length_ticks, nm89_u8 loop,
                    nm89_i16 *out_clip_id);
int nm89_clip_add_track(nm89_rig *rig, nm89_i16 part_id,
                        nm89_u8 property, nm89_u8 interpolation,
                        nm89_i16 *out_track_id);
int nm89_track_add_key(nm89_rig *rig, nm89_i16 track_id,
                       nm89_u16 tick, nm89_fx value);
int nm89_clip_add_event(nm89_rig *rig,
                        const nm89_clip_event *event_value);
int nm89_clip_end(nm89_rig *rig);

int nm89_action_bind(nm89_rig *rig, nm89_i16 action_id,
                     nm89_i16 clip_id, nm89_u8 policy);
int nm89_trigger_action(nm89_rig *rig, nm89_i16 action_id);
int nm89_play(nm89_rig *rig, nm89_i16 clip_id, nm89_u8 restart);
void nm89_stop(nm89_rig *rig);
int nm89_seek(nm89_rig *rig, nm89_u16 tick);
int nm89_update(nm89_rig *rig, nm89_u16 delta_ticks);
int nm89_update_from_clock(nm89_rig *rig);

/* Complete evaluation pipeline. */
int nm89_evaluate(nm89_rig *rig);
int nm89_update_world(nm89_rig *rig);
int nm89_flush(nm89_rig *rig);
int nm89_step(nm89_rig *rig, nm89_u16 delta_ticks, nm89_u8 flush_output);

/* Queries and provider-aware math. */
const nm89_pose *nm89_part_get_pose(const nm89_rig *rig,
                                    nm89_i16 part_id);
const nm89_matrix *nm89_part_get_world(const nm89_rig *rig,
                                       nm89_i16 part_id);
void nm89_rig_transform_point(const nm89_rig *rig,
                              const nm89_matrix *matrix,
                              nm89_fx x, nm89_fx y, nm89_fx z,
                              nm89_fx *out_x,
                              nm89_fx *out_y,
                              nm89_fx *out_z);

#ifdef __cplusplus
}
#endif

#endif
