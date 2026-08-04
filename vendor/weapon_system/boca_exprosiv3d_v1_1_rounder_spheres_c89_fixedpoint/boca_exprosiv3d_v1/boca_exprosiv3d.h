#ifndef BOCA_EXPROSIV3D_H
#define BOCA_EXPROSIV3D_H

/*
 * boca_exprosiv3d v1
 * Physics-inspired procedural 3D muzzle-blast meshes.
 *
 * Strict C89.
 * Fixed-point arithmetic only.
 * No malloc, realloc, free, heap, float or double.
 * No textures, sprites or billboards.
 * Renderer agnostic: the caller supplies a reusable mesh buffer.
 */

#define BEX3D_VERSION_MAJOR 1
#define BEX3D_VERSION_MINOR 1

#define BEX3D_FP_SHIFT 10
#define BEX3D_FP_ONE   (1L << BEX3D_FP_SHIFT)
#define BEX3D_FP_HALF  (BEX3D_FP_ONE / 2L)

#define BEX3D_INT(value) ((BEX3D_Fixed)(value) * BEX3D_FP_ONE)
#define BEX3D_CM(value)  BEX3D_INT(value)
#define BEX3D_MM(value)  (((BEX3D_Fixed)(value) * BEX3D_FP_ONE) / 10L)

#define BEX3D_MAX_PRIMITIVES 40
#define BEX3D_MAX_SIDES      16
#define BEX3D_MAX_STACKS     8

#define BEX3D_PI_ANGLE       128
#define BEX3D_HALF_PI_ANGLE   64
#define BEX3D_FULL_ANGLE      256

#define BEX3D_PRIM_FRUSTUM 1
#define BEX3D_PRIM_SPHERE  2
#define BEX3D_PRIM_RING    3

#define BEX3D_CAP_START 1UL
#define BEX3D_CAP_END   2UL

#define BEX3D_ROLE_PRIMARY_JET      1
#define BEX3D_ROLE_SHOCK_CELL       2
#define BEX3D_ROLE_MACH_DISK        3
#define BEX3D_ROLE_VORTEX_RING      4
#define BEX3D_ROLE_SECONDARY_FLASH  5
#define BEX3D_ROLE_GAS_CLOUD        6
#define BEX3D_ROLE_DEVICE_JET       7
#define BEX3D_ROLE_CYLINDER_GAP     8

#define BEX3D_DEVICE_NONE             0
#define BEX3D_DEVICE_FLASH_HIDER      1
#define BEX3D_DEVICE_BRAKE_HORIZONTAL 2
#define BEX3D_DEVICE_BRAKE_RADIAL     3
#define BEX3D_DEVICE_SUPPRESSOR       4
#define BEX3D_DEVICE_REVOLVER_GAP     5

#define BEX3D_PROFILE_PISTOL            0
#define BEX3D_PROFILE_MACHINE_GUN       1
#define BEX3D_PROFILE_SHOTGUN           2
#define BEX3D_PROFILE_MAGNUM_AUTO       3
#define BEX3D_PROFILE_MAGNUM_REVOLVER   4
#define BEX3D_PROFILE_PRECISION_RIFLE   5
#define BEX3D_PROFILE_PRECISION_BRAKE   6
#define BEX3D_PROFILE_SUPPRESSED        7
#define BEX3D_PROFILE_COUNT             8

#define BEX3D_BUILD_OK         0
#define BEX3D_BUILD_TRUNCATED  1
#define BEX3D_BUILD_BAD_BUFFER 2

typedef long BEX3D_Fixed;

typedef struct BEX3D_Color
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} BEX3D_Color;

typedef struct BEX3D_Vertex
{
    BEX3D_Fixed x;
    BEX3D_Fixed y;
    BEX3D_Fixed z;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} BEX3D_Vertex;

typedef struct BEX3D_MeshBuffer
{
    BEX3D_Vertex *vertices;
    unsigned short *indices;
    unsigned long vertex_capacity;
    unsigned long index_capacity;
    unsigned long vertex_count;
    unsigned long index_count;
    int truncated;
} BEX3D_MeshBuffer;

/*
 * Normalized physical controls use Q10:
 * 0 = absent, 1024 = nominal, values above 1024 are allowed.
 * Distances are centimeters in Q10.
 */
typedef struct BEX3D_Profile
{
    BEX3D_Fixed muzzle_pressure;
    BEX3D_Fixed gas_mass;
    BEX3D_Fixed residual_fuel;
    BEX3D_Fixed turbulence;
    BEX3D_Fixed barrel_efficiency;
    BEX3D_Fixed radial_spread;
    BEX3D_Fixed axial_bias;
    BEX3D_Fixed bore_radius;

    int minimum_shock_cells;
    int maximum_shock_cells;
    int minimum_vortex_rings;
    int maximum_vortex_rings;
    int gas_cloud_count;
    int secondary_flash_chance;
    int primitive_budget;
    int device;

    unsigned long event_duration_us;
} BEX3D_Profile;

typedef struct BEX3D_Primitive
{
    int type;
    int role;
    int sides;
    int stacks;
    unsigned long flags;

    unsigned long start_us;
    unsigned long duration_us;

    BEX3D_Fixed x0;
    BEX3D_Fixed y0;
    BEX3D_Fixed z0;
    BEX3D_Fixed x1;
    BEX3D_Fixed y1;
    BEX3D_Fixed z1;

    BEX3D_Fixed radius_a0;
    BEX3D_Fixed radius_a1;
    BEX3D_Fixed radius_b0;
    BEX3D_Fixed radius_b1;
    BEX3D_Fixed length0;
    BEX3D_Fixed length1;

    int yaw;
    int pitch;
    int roll0;
    int roll1;

    BEX3D_Color hot_color;
    BEX3D_Color cool_color;
    int alpha0;
    int alpha1;
} BEX3D_Primitive;

typedef struct BEX3D_State
{
    unsigned long rng_state;
    BEX3D_Profile profile;

    int active;
    unsigned long age_us;
    unsigned long duration_us;
    int primitive_count;

    BEX3D_Fixed origin_x;
    BEX3D_Fixed origin_y;
    BEX3D_Fixed origin_z;

    /* Caller-provided orthonormal weapon basis, all Q10. */
    BEX3D_Fixed right_x;
    BEX3D_Fixed right_y;
    BEX3D_Fixed right_z;
    BEX3D_Fixed up_x;
    BEX3D_Fixed up_y;
    BEX3D_Fixed up_z;
    BEX3D_Fixed forward_x;
    BEX3D_Fixed forward_y;
    BEX3D_Fixed forward_z;

    BEX3D_Primitive primitives[BEX3D_MAX_PRIMITIVES];
} BEX3D_State;

void bex3d_default_profile(BEX3D_Profile *profile, int profile_id);
void bex3d_init(BEX3D_State *state, unsigned long seed);
void bex3d_reset(BEX3D_State *state);
void bex3d_set_profile(BEX3D_State *state, const BEX3D_Profile *profile);
void bex3d_set_origin(BEX3D_State *state,
                      BEX3D_Fixed x,
                      BEX3D_Fixed y,
                      BEX3D_Fixed z);
void bex3d_set_basis(BEX3D_State *state,
                     BEX3D_Fixed right_x,
                     BEX3D_Fixed right_y,
                     BEX3D_Fixed right_z,
                     BEX3D_Fixed up_x,
                     BEX3D_Fixed up_y,
                     BEX3D_Fixed up_z,
                     BEX3D_Fixed forward_x,
                     BEX3D_Fixed forward_y,
                     BEX3D_Fixed forward_z);

void bex3d_fire(BEX3D_State *state);
void bex3d_update_us(BEX3D_State *state, unsigned long delta_us);
int bex3d_build_mesh(const BEX3D_State *state, BEX3D_MeshBuffer *mesh);

BEX3D_Fixed bex3d_mul(BEX3D_Fixed a, BEX3D_Fixed b);
BEX3D_Fixed bex3d_div(BEX3D_Fixed a, BEX3D_Fixed b);
BEX3D_Fixed bex3d_sin(int angle);
BEX3D_Fixed bex3d_cos(int angle);

int bex3d_visible_primitive_count(const BEX3D_State *state);
const BEX3D_Primitive *bex3d_get_primitive(const BEX3D_State *state,
                                           int index);

#endif
