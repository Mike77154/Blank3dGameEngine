#ifndef MORETHANONE89_H
#define MORETHANONE89_H

#ifdef __cplusplus
extern "C" {
#endif

#define MTO89_MAX_STAGES 8

#define MTO89_OVERRIDE_PROJECTILE_ID      0x0001UL
#define MTO89_OVERRIDE_PROJECTILE_MESH_ID 0x0002UL
#define MTO89_OVERRIDE_DAMAGE_Q16         0x0004UL
#define MTO89_OVERRIDE_SPEED_Q16          0x0008UL
#define MTO89_OVERRIDE_RADIUS_Q16         0x0010UL
#define MTO89_OVERRIDE_MESH_SCALE_Q16     0x0020UL
#define MTO89_OVERRIDE_LIFE_MS            0x0040UL

typedef struct mto89_stage_s {
    unsigned int min_ms;
    int projectile_id;
    int projectile_mesh_id;
    long damage_q16;
    long speed_q16;
    long radius_q16;
    long mesh_scale_q16;
    unsigned short life_ms;
    unsigned long override_mask;
} mto89_stage;

typedef struct mto89_profile_s {
    int enabled;
    unsigned int activation_ms;
    int stage_count;
    mto89_stage stages[MTO89_MAX_STAGES];
} mto89_profile;

typedef struct mto89_result_s {
    int active;
    int stage_index;
    mto89_stage stage;
} mto89_result;

void mto89_profile_init(mto89_profile *profile);
int mto89_set_stage(mto89_profile *profile, int index,
                    const mto89_stage *stage);
int mto89_validate(const mto89_profile *profile);
int mto89_resolve(const mto89_profile *profile, unsigned int hold_ms,
                  mto89_result *out_result);

#ifdef __cplusplus
}
#endif

#endif
