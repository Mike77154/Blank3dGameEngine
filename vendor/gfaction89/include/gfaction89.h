#ifndef GFACTION89_H
#define GFACTION89_H

#include "gfaction89_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void gfa_init(GFA_World *world);
void gfa_set_scratch_arena(GFA_World *world, unsigned char *data, int capacity);
void *gfa_arena_push(GFA_Arena *arena, int bytes);
void gfa_arena_reset(GFA_Arena *arena);

int gfa_register_faction(GFA_World *world, const char *name);
int gfa_register_team(GFA_World *world, const char *name);
int gfa_register_role(GFA_World *world, const char *name);
int gfa_register_tag(GFA_World *world, const char *name);

int gfa_find_faction(const GFA_World *world, const char *name);
int gfa_find_team(const GFA_World *world, const char *name);
int gfa_find_role(const GFA_World *world, const char *name);
int gfa_find_tag(const GFA_World *world, const char *name);

const char *gfa_faction_name(const GFA_World *world, int id);
const char *gfa_team_name(const GFA_World *world, int id);
const char *gfa_role_name(const GFA_World *world, int id);
const char *gfa_tag_name(const GFA_World *world, int id);
const char *gfa_disposition_name(int disposition);
int gfa_parse_disposition(const char *text);
int gfa_parse_flags(const char *text);

int gfa_ensure_entity(GFA_World *world, int entity_id);
GFA_Entity *gfa_get_entity(GFA_World *world, int entity_id);
const GFA_Entity *gfa_get_entity_const(const GFA_World *world, int entity_id);

int gfa_set_entity_faction(GFA_World *world, int entity_id, int faction_id);
int gfa_set_entity_team(GFA_World *world, int entity_id, int team_id);
int gfa_set_entity_role(GFA_World *world, int entity_id, int role_id);
int gfa_add_entity_tag(GFA_World *world, int entity_id, int tag_id);
int gfa_clear_entity_tag(GFA_World *world, int entity_id, int tag_id);
int gfa_entity_has_tag(const GFA_World *world, int entity_id, int tag_id);
int gfa_set_entity_targetable(GFA_World *world, int entity_id, int can_be_targeted);
int gfa_set_entity_alive(GFA_World *world, int entity_id, int alive);
int gfa_set_entity_stats(GFA_World *world, int entity_id, int threat, int morale);

int gfa_set_relation(GFA_World *world, int src_faction, int dst_faction, int disposition, int priority, int flags, int score_bias);
int gfa_add_tag_rule(GFA_World *world, int src_tag, int dst_tag, int disposition, int priority, int flags, int score_bias);
int gfa_add_role_rule(GFA_World *world, int src_role, int dst_role, int disposition, int priority, int flags, int score_bias);
int gfa_add_override(GFA_World *world, int src_entity, int dst_entity, int disposition, int priority, int flags, int score_bias);
int gfa_clear_override(GFA_World *world, int src_entity, int dst_entity);

GFA_Decision gfa_eval(const GFA_World *world, int src_entity, int dst_entity, const GFA_Context *ctx);
int gfa_choose_best_target(const GFA_World *world, int src_entity, const int *candidates, int candidate_count, const GFA_Context *ctx, GFA_Decision *out_decision);
int gfa_filter_candidates(const GFA_World *world, int src_entity, const int *candidates, int candidate_count, int *out_candidates, int out_max, int required_flags, const GFA_Context *ctx);

int gfa_add_trigger(GFA_World *world, const GFA_Trigger *trigger_data);
int gfa_fire_event(GFA_World *world, const GFA_Event *event_data);
int gfa_add_hooks(GFA_World *world, const GFA_Hooks *hooks);

int gfa_load_ini_text(GFA_World *world, const char *text);
int gfa_load_ini_file(GFA_World *world, const char *path);

void gfa_debug_print_entity(const GFA_World *world, int entity_id);
void gfa_debug_print_decision(const GFA_World *world, int src_entity, int dst_entity, const GFA_Decision *decision);

#ifdef __cplusplus
}
#endif

#endif
