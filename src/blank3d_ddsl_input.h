#ifndef BLANK3D_DDSL_INPUT_H
#define BLANK3D_DDSL_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_DDSL_INPUT_MAX_REFS 128
#define B3D_DDSL_INPUT_STORE_NAME_CAP 32
#define B3D_DDSL_INPUT_STATE_CAP 16
#define B3D_DDSL_INPUT_CONTROL_CAP 64

typedef struct Blank3DDdslInputRefTag {
    char store_name[B3D_DDSL_INPUT_STORE_NAME_CAP];
    char state[B3D_DDSL_INPUT_STATE_CAP];
    char control[B3D_DDSL_INPUT_CONTROL_CAP];
} Blank3DDdslInputRef;

typedef struct Blank3DDdslInputRegistryTag {
    Blank3DDdslInputRef refs[B3D_DDSL_INPUT_MAX_REFS];
    int count;
} Blank3DDdslInputRegistry;

void blank3d_ddsl_input_registry_init(Blank3DDdslInputRegistry *registry);
int blank3d_ddsl_input_preprocess(const char *source,
                                  char *output,
                                  unsigned int output_capacity,
                                  Blank3DDdslInputRegistry *registry,
                                  char *error,
                                  unsigned int error_capacity);

#ifdef __cplusplus
}
#endif

#endif
