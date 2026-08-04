#ifndef BLANK3D_OBJECTS_H
#define BLANK3D_OBJECTS_H

#include "gfo_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_OBJECT_MAX_ENTITIES 16
#define B3D_OBJECT_PATH_CAP 128
#define B3D_OBJECT_NAME_CAP 48
#define B3D_OBJECT_GFO_SOURCE_CAP 16384
#define B3D_OBJECT_GFO_ARENA_CAP 524288

typedef enum Blank3DMeshKindTag {
    B3D_MESH_NONE = 0,
    B3D_MESH_CUBE,
    B3D_MESH_SPHERE,
    B3D_MESH_CAPSULE,
    B3D_MESH_COMPOSED,
    B3D_MESH_CUSTOMCALLED,
    B3D_MESH_EXTERNAL
} Blank3DMeshKind;

typedef enum Blank3DLogicKindTag {
    B3D_LOGIC_NONE = 0,
    B3D_LOGIC_DDSL2,
    B3D_LOGIC_FPIL,
    B3D_LOGIC_RPYL
} Blank3DLogicKind;

typedef struct Blank3DObjectInitTag {
    char name[B3D_OBJECT_NAME_CAP];
    char gfo_path[B3D_OBJECT_PATH_CAP];
    char logic_path[B3D_OBJECT_PATH_CAP];
    char mesh_spec[B3D_OBJECT_PATH_CAP];
    char draw_script[B3D_OBJECT_PATH_CAP];
    int hp;
    long speed_q16;
    long scale_x_q16;
    long scale_y_q16;
    long scale_z_q16;
    Blank3DMeshKind mesh_kind;
    Blank3DLogicKind logic_kind;
} Blank3DObjectInit;

typedef struct Blank3DObjectEntityTag {
    int alive;
    unsigned long entity_id;
    void *native_entity;
    Blank3DObjectInit init;
    char gfo_source[B3D_OBJECT_GFO_SOURCE_CAP];
    unsigned char gfo_arena[B3D_OBJECT_GFO_ARENA_CAP];
    gfo_ctx gfo;
    gfo_u16 gfo_instance;
} Blank3DObjectEntity;

typedef struct Blank3DObjectHostTag {
    void *user;
    void (*run_ddsl2)(void *user, unsigned long entity_id,
                      void *native_entity, const char *path);
    void (*run_fpil)(void *user, unsigned long entity_id,
                     void *native_entity, const char *path);
    void (*run_rpyl)(void *user, unsigned long entity_id,
                     void *native_entity, const char *path);
    void (*draw_mesh)(void *user, unsigned long entity_id,
                      void *native_entity, const Blank3DObjectInit *init);
    void (*draw_script)(void *user, unsigned long entity_id,
                        void *native_entity, const char *language,
                        const char *code, unsigned int code_len);
    void (*invoke)(void *user, unsigned long entity_id,
                   void *native_entity, const char *name,
                   const gfo_value *argv, unsigned int argc);
    void (*handle)(void *user, unsigned long entity_id,
                   void *native_entity, const char *name);
} Blank3DObjectHost;

typedef struct Blank3DObjectsTag {
    Blank3DObjectHost host;
    Blank3DObjectEntity entities[B3D_OBJECT_MAX_ENTITIES];
    unsigned int count;
    char status[192];
} Blank3DObjects;

void blank3d_objects_init(Blank3DObjects *objects,
                          const Blank3DObjectHost *host);
int blank3d_objects_spawn(Blank3DObjects *objects,
                          const char *ini_path,
                          unsigned long entity_id,
                          void *native_entity,
                          unsigned int *out_slot);
void blank3d_objects_tick(Blank3DObjects *objects);
void blank3d_objects_render(Blank3DObjects *objects);
void blank3d_objects_kill(Blank3DObjects *objects, unsigned int slot);
const Blank3DObjectEntity *blank3d_objects_get(
    const Blank3DObjects *objects, unsigned int slot);
const char *blank3d_objects_status(const Blank3DObjects *objects);

#ifdef __cplusplus
}
#endif
#endif
