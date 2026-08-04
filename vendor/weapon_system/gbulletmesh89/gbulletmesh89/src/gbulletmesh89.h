#ifndef GBULLETMESH89_H
#define GBULLETMESH89_H

#ifdef __cplusplus
extern "C" {
#endif

#define GBM_FIX_ONE 65536L
#define GBM_SEGMENTS 12

#define GBM_OK 0
#define GBM_ERR_NULL 1
#define GBM_ERR_CAPACITY 2
#define GBM_ERR_BAD_TYPE 3

typedef long gbm_fix;

typedef struct GBM_Vertex_s {
    gbm_fix x;
    gbm_fix y;
    gbm_fix z;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} GBM_Vertex;

typedef struct GBM_Tri_s {
    unsigned short a;
    unsigned short b;
    unsigned short c;
} GBM_Tri;

typedef struct GBM_Mesh_s {
    GBM_Vertex *v;
    unsigned short v_cap;
    unsigned short v_count;
    GBM_Tri *t;
    unsigned short t_cap;
    unsigned short t_count;
    int error;
} GBM_Mesh;

typedef enum GBM_AmmoType_e {
    GBM_AMMO_PISTOL = 1,
    GBM_AMMO_SHOTGUN = 2,
    GBM_AMMO_SNIPER = 3,
    GBM_AMMO_MAGNUM = 4,
    GBM_AMMO_MACHINEGUN = 5
} GBM_AmmoType;

void gbm_mesh_init(GBM_Mesh *m, GBM_Vertex *v, unsigned short v_cap, GBM_Tri *t, unsigned short t_cap);
void gbm_mesh_reset(GBM_Mesh *m);

gbm_fix gbm_fix_from_int(int x);
gbm_fix gbm_fix_ratio(int num, int den);

int gbm_build_shell(GBM_Mesh *m, int ammo_type);
int gbm_build_projectile(GBM_Mesh *m, int ammo_type);
int gbm_build_shotgun_pellet(GBM_Mesh *m);
int gbm_build_full_round(GBM_Mesh *m, int ammo_type);
int gbm_build_mg_link(GBM_Mesh *m);

void gbm_mesh_translate(GBM_Mesh *m, gbm_fix tx, gbm_fix ty, gbm_fix tz);

#ifdef __cplusplus
}
#endif

#endif
