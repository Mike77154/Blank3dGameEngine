#ifndef SOQUETE3D_H
#define SOQUETE3D_H

/*
 * Soquete3D 1.0
 * A tiny, engine-agnostic 3D pose/socket registry.
 *
 * Design constraints:
 * - ISO C89 source style
 * - no malloc/realloc/free, no heap ownership
 * - no float or double
 * - Q16.16 fixed point only
 * - no world management, no scene graph, no recursive parent chains
 */

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SOQ3D_MAX_THINGS
#define SOQ3D_MAX_THINGS 256
#endif

#ifndef SOQ3D_MAX_SOCKETS
#define SOQ3D_MAX_SOCKETS 512
#endif

#define SOQ3D_FRAC_BITS 16
#define SOQ3D_FX_ONE ((soq3d_fx)65536)
#define SOQ3D_KEY_NONE ((soq3d_key)0U)
#define SOQ3D_STAMP_ANY ((soq3d_stamp)UINT_MAX)

/* Soquete3D intentionally requires a 32-bit unsigned int. */
typedef char soq3d_requires_32_bit_uint[(UINT_MAX == 4294967295U) ? 1 : -1];

typedef signed int soq3d_fx;
typedef unsigned int soq3d_key;
typedef unsigned int soq3d_stamp;

typedef enum soq3d_result {
    SOQ3D_OK = 0,
    SOQ3D_ERR_BAD_ARG = -1,
    SOQ3D_ERR_KEY_RESERVED = -2,
    SOQ3D_ERR_FULL = -3,
    SOQ3D_ERR_NOT_FOUND = -4,
    SOQ3D_ERR_STALE = -5,
    SOQ3D_ERR_BAD_AXIS = -6
} soq3d_result;

typedef enum soq3d_axis {
    SOQ3D_AXIS_X = 0,
    SOQ3D_AXIS_Y = 1,
    SOQ3D_AXIS_Z = 2
} soq3d_axis;

typedef struct soq3d_vec3 {
    soq3d_fx x;
    soq3d_fx y;
    soq3d_fx z;
} soq3d_vec3;

/* Row-major 3x3 basis. It may contain rotation and scale. */
typedef struct soq3d_basis3 {
    soq3d_fx m00;
    soq3d_fx m01;
    soq3d_fx m02;
    soq3d_fx m10;
    soq3d_fx m11;
    soq3d_fx m12;
    soq3d_fx m20;
    soq3d_fx m21;
    soq3d_fx m22;
} soq3d_basis3;

typedef struct soq3d_pose {
    soq3d_vec3 position;
    soq3d_basis3 basis;
} soq3d_pose;

typedef struct soq3d_thing_record {
    soq3d_key key;
    soq3d_stamp stamp;
    soq3d_pose world_pose;
    unsigned char used;
} soq3d_thing_record;

typedef enum soq3d_socket_mode {
    SOQ3D_SOCKET_UNUSED = 0,
    SOQ3D_SOCKET_LOCAL_TO_THING = 1,
    SOQ3D_SOCKET_WORLD_PUBLISHED = 2
} soq3d_socket_mode;

typedef struct soq3d_socket_record {
    soq3d_key key;
    soq3d_key owner_key;
    soq3d_stamp stamp;
    soq3d_pose pose;
    unsigned char mode;
    unsigned char used;
} soq3d_socket_record;

typedef struct soq3d_context {
    soq3d_stamp current_stamp;
    soq3d_thing_record things[SOQ3D_MAX_THINGS];
    soq3d_socket_record sockets[SOQ3D_MAX_SOCKETS];
} soq3d_context;

/* Lifecycle and frame stamp. */
void soq3d_init(soq3d_context *ctx);
void soq3d_begin_frame(soq3d_context *ctx, soq3d_stamp stamp);
soq3d_stamp soq3d_current_stamp(const soq3d_context *ctx);

/* Fixed-point helpers. All arithmetic saturates on overflow. */
soq3d_fx soq3d_fx_from_int(int value);
int soq3d_fx_to_int_floor(soq3d_fx value);
soq3d_fx soq3d_fx_add_sat(soq3d_fx a, soq3d_fx b);
soq3d_fx soq3d_fx_mul(soq3d_fx a, soq3d_fx b);

/* Pose helpers. */
soq3d_vec3 soq3d_vec3_make(soq3d_fx x, soq3d_fx y, soq3d_fx z);
soq3d_basis3 soq3d_basis_identity(void);
soq3d_pose soq3d_pose_identity(void);
soq3d_pose soq3d_pose_make(soq3d_vec3 position, soq3d_basis3 basis);
soq3d_vec3 soq3d_basis_transform_point(const soq3d_basis3 *basis,
                                       const soq3d_vec3 *point);
soq3d_pose soq3d_pose_compose(const soq3d_pose *parent_world,
                              const soq3d_pose *local_pose);

/* Caller-owned stable keys. Hash helper is optional; key 0 is reserved. */
soq3d_key soq3d_key_from_cstr(const char *text);

/* Dynamic things: publish their current world pose once per simulation stamp. */
soq3d_result soq3d_publish_thing(soq3d_context *ctx,
                                 soq3d_key key,
                                 const soq3d_pose *world_pose);
soq3d_result soq3d_remove_thing(soq3d_context *ctx, soq3d_key key);
soq3d_result soq3d_get_thing(const soq3d_context *ctx,
                             soq3d_key key,
                             soq3d_stamp required_stamp,
                             soq3d_pose *out_world_pose);

/*
 * Persistent local socket definition. The owner must be a thing, never another
 * socket. Querying resolves exactly one local->world composition: no graph.
 */
soq3d_result soq3d_define_local_socket(soq3d_context *ctx,
                                       soq3d_key socket_key,
                                       soq3d_key owner_thing_key,
                                       const soq3d_pose *local_pose);

/*
 * Direct world publication for animation/bone systems that already resolve the
 * socket. Must be republished for every stamp when exact freshness is required.
 */
soq3d_result soq3d_publish_world_socket(soq3d_context *ctx,
                                        soq3d_key socket_key,
                                        const soq3d_pose *world_pose);

soq3d_result soq3d_remove_socket(soq3d_context *ctx, soq3d_key socket_key);
soq3d_result soq3d_get_socket(const soq3d_context *ctx,
                              soq3d_key socket_key,
                              soq3d_stamp required_stamp,
                              soq3d_pose *out_world_pose);

/* Convenience: read the translation component on the caller's chosen up axis. */
soq3d_result soq3d_get_thing_height(const soq3d_context *ctx,
                                    soq3d_key key,
                                    soq3d_stamp required_stamp,
                                    soq3d_axis up_axis,
                                    soq3d_fx *out_height);
soq3d_result soq3d_get_socket_height(const soq3d_context *ctx,
                                     soq3d_key key,
                                     soq3d_stamp required_stamp,
                                     soq3d_axis up_axis,
                                     soq3d_fx *out_height);

/* Introspection only; no ownership semantics. */
unsigned int soq3d_count_things(const soq3d_context *ctx);
unsigned int soq3d_count_sockets(const soq3d_context *ctx);
const char *soq3d_result_string(soq3d_result result);

#ifdef __cplusplus
}
#endif

#endif
