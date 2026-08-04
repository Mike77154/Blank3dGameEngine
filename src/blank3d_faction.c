#include "blank3d_faction.h"
#include "gfaction89_ini.h"

#include <stdio.h>
#include <string.h>

static void b3d_faction_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] != '\0' && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int b3d_faction_name_id(const GFA_World *world,
                               const char *kind,
                               const char *name)
{
    if (!world || !kind || !name || !*name) return GFA_INVALID_ID;
    if (strcmp(kind, "faction") == 0) return gfa_find_faction(world, name);
    if (strcmp(kind, "team") == 0) return gfa_find_team(world, name);
    if (strcmp(kind, "role") == 0) return gfa_find_role(world, name);
    return GFA_INVALID_ID;
}

int blank3d_faction_init(Blank3DFactionSystem *system, const char *ini_path)
{
    if (!system || !ini_path || !*ini_path) return 0;
    memset(system, 0, sizeof(*system));
    gfa_init(&system->world);
    if (!gfa_load_ini_file(&system->world, ini_path)) {
        sprintf(system->status, "gfaction89: failed to load %.120s", ini_path);
        return 0;
    }
    b3d_faction_copy(system->source_path,
                     (unsigned int)sizeof(system->source_path), ini_path);
    system->initialized = 1;
    sprintf(system->status, "gfaction89: loaded %.120s", ini_path);
    return 1;
}

int blank3d_faction_register_actor(Blank3DFactionSystem *system,
                                  int entity_id,
                                  const char *faction,
                                  const char *team,
                                  const char *role,
                                  const char *tags_csv,
                                  int threat,
                                  int morale,
                                  int alive,
                                  int targetable)
{
    int faction_id;
    int team_id;
    int role_id;
    int tag_id;
    int i;
    int count;
    char tokens[GFA_MAX_TOKENS][GFA_MAX_NAME];
    if (!system || !system->initialized) return 0;
    faction_id = b3d_faction_name_id(&system->world, "faction", faction);
    team_id = b3d_faction_name_id(&system->world, "team", team);
    role_id = b3d_faction_name_id(&system->world, "role", role);
    if (faction_id < 0 || team_id < 0 || role_id < 0) return 0;
    if (!gfa_ensure_entity(&system->world, entity_id)) return 0;
    if (!gfa_set_entity_faction(&system->world, entity_id, faction_id)) return 0;
    if (!gfa_set_entity_team(&system->world, entity_id, team_id)) return 0;
    if (!gfa_set_entity_role(&system->world, entity_id, role_id)) return 0;
    (void)gfa_set_entity_stats(&system->world, entity_id, threat, morale);
    (void)gfa_set_entity_alive(&system->world, entity_id, alive ? 1 : 0);
    (void)gfa_set_entity_targetable(&system->world, entity_id,
                                    targetable ? 1 : 0);
    if (tags_csv && *tags_csv) {
        count = gfa_ini_tokenize_csv(tags_csv, tokens, GFA_MAX_TOKENS);
        for (i = 0; i < count; ++i) {
            tag_id = gfa_find_tag(&system->world, tokens[i]);
            if (tag_id >= 0)
                (void)gfa_add_entity_tag(&system->world, entity_id, tag_id);
        }
    }
    return 1;
}

int blank3d_faction_sync_actor(Blank3DFactionSystem *system,
                              int entity_id,
                              int threat,
                              int morale,
                              int alive,
                              int targetable)
{
    if (!system || !system->initialized) return 0;
    if (!gfa_get_entity(&system->world, entity_id)) return 0;
    (void)gfa_set_entity_stats(&system->world, entity_id, threat, morale);
    (void)gfa_set_entity_alive(&system->world, entity_id, alive ? 1 : 0);
    (void)gfa_set_entity_targetable(&system->world, entity_id,
                                    targetable ? 1 : 0);
    return 1;
}

GFA_Decision blank3d_faction_decide(const Blank3DFactionSystem *system,
                                    int source_entity,
                                    int target_entity,
                                    const GFA_Context *context)
{
    GFA_Decision empty;
    memset(&empty, 0, sizeof(empty));
    if (!system || !system->initialized) return empty;
    return gfa_eval(&system->world, source_entity, target_entity, context);
}

int blank3d_faction_can_attack(const Blank3DFactionSystem *system,
                              int source_entity,
                              int target_entity,
                              const GFA_Context *context)
{
    GFA_Decision decision;
    decision = blank3d_faction_decide(system, source_entity,
                                      target_entity, context);
    return decision.can_attack ? 1 : 0;
}

int blank3d_faction_are_allies(const Blank3DFactionSystem *system,
                               int source_entity,
                               int target_entity)
{
    GFA_Decision decision;
    decision = blank3d_faction_decide(system, source_entity,
                                      target_entity, (const GFA_Context *)0);
    return (decision.can_assist || decision.can_protect ||
            decision.can_follow) && !decision.can_attack;
}

int blank3d_faction_choose_attack_target(
    const Blank3DFactionSystem *system,
    int source_entity,
    const Blank3DFactionCandidate *candidates,
    int candidate_count,
    GFA_Decision *out_decision)
{
    int i;
    int best_entity;
    int best_score;
    GFA_Decision decision;
    if (!system || !system->initialized || !candidates ||
        candidate_count <= 0) return GFA_INVALID_ID;
    best_entity = GFA_INVALID_ID;
    best_score = -2147480000;
    for (i = 0; i < candidate_count; ++i) {
        if (candidates[i].entity_id == source_entity) continue;
        decision = gfa_eval(&system->world, source_entity,
                            candidates[i].entity_id,
                            &candidates[i].context);
        if (decision.can_attack && decision.score_fp > best_score) {
            best_score = decision.score_fp;
            best_entity = candidates[i].entity_id;
            if (out_decision) *out_decision = decision;
        }
    }
    return best_entity;
}

static int b3d_faction_distance_units(const GFA_Context *context)
{
    if (!context || context->distance_fp <= 0) return 0;
    return GFA_FP_TO_INT(context->distance_fp);
}

int blank3d_faction_choose_attack_target_stable(
    const Blank3DFactionSystem *system,
    int source_entity,
    const Blank3DFactionCandidate *candidates,
    int candidate_count,
    int current_target,
    int distance_weight,
    int current_target_bonus,
    GFA_Decision *out_decision)
{
    int i;
    int best_entity;
    int best_score;
    int best_distance;
    int candidate_distance;
    int adjusted_score;
    int prefer_candidate;
    GFA_Decision decision;

    if (!system || !system->initialized || !candidates ||
        candidate_count <= 0) return GFA_INVALID_ID;
    if (distance_weight < 0) distance_weight = 0;
    if (current_target_bonus < 0) current_target_bonus = 0;

    best_entity = GFA_INVALID_ID;
    best_score = -2147480000;
    best_distance = 2147480000;

    for (i = 0; i < candidate_count; ++i) {
        if (candidates[i].entity_id == source_entity) continue;
        decision = gfa_eval(&system->world, source_entity,
                            candidates[i].entity_id,
                            &candidates[i].context);
        if (!decision.can_attack) continue;

        candidate_distance = b3d_faction_distance_units(
            &candidates[i].context);
        adjusted_score = decision.score_fp;
        if (distance_weight > 0 && candidate_distance > 0)
            adjusted_score -= GFA_FP_FROM_INT(
                candidate_distance * distance_weight);
        if (candidates[i].entity_id == current_target)
            adjusted_score += GFA_FP_FROM_INT(current_target_bonus);

        prefer_candidate = adjusted_score > best_score;
        if (!prefer_candidate && adjusted_score == best_score) {
            if (candidates[i].entity_id == current_target &&
                best_entity != current_target)
                prefer_candidate = 1;
            else if (candidate_distance < best_distance)
                prefer_candidate = 1;
            else if (candidate_distance == best_distance &&
                     (best_entity == GFA_INVALID_ID ||
                      candidates[i].entity_id < best_entity))
                prefer_candidate = 1;
        }

        if (prefer_candidate) {
            best_score = adjusted_score;
            best_distance = candidate_distance;
            best_entity = candidates[i].entity_id;
            decision.score_fp = adjusted_score;
            if (out_decision) *out_decision = decision;
        }
    }
    return best_entity;
}

const char *blank3d_faction_status(const Blank3DFactionSystem *system)
{
    return system ? system->status : "gfaction89: unavailable";
}
