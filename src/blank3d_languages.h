#ifndef BLANK3D_LANGUAGES_H
#define BLANK3D_LANGUAGES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DLanguageHostTag {
    void *user;
    int (*key_down)(void *user, const char *key_name); /* legacy hold query */
    int (*input_query)(void *user, const char *state_name,
                       const char *control_name);
    void (*ddsl_action)(void *user, const char *action_name,
                        long value_fixed, const char *value_text);
    void (*rpyl_command)(void *user, const char *command,
                         const char **args, int argc);
    int (*fpil_condition)(void *user, void *entity,
                          const char *condition,
                          long value_q16, const char *value_text,
                          int has_value);
    void (*fpil_action)(void *user, void *entity,
                        const char *action,
                        long value_q16, const char *value_text,
                        int has_value);
} Blank3DLanguageHost;

void blank3d_languages_init(const Blank3DLanguageHost *host);
int blank3d_languages_reload_ddsl2(const char *path);
int blank3d_languages_tick_ddsl2(void);
int blank3d_languages_reload_fpil(const char *path);
int blank3d_languages_tick_fpil(void *entity);
int blank3d_languages_tick_fpil_path(const char *path, void *entity);
int blank3d_languages_reload_fpil_path(const char *path);
int blank3d_languages_run_rpyl(const char *path);
const char *blank3d_languages_status(void);

#ifdef __cplusplus
}
#endif

#endif
