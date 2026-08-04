#ifndef BLANK3D_FACTION_H
#define BLANK3D_FACTION_H

#include "gfaction89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_FACTION_PLAYER_ENTITY_ID 1
#define B3D_FACTION_MAX_CANDIDATES 64

typedef struct Blank3DFactionSystemTag {
    GFA_World world;
    int initialized;
    char source_path[160];
    char status[160];
} Blank3DFactionSystem;

typedef struct Blank3DFactionCandidateTag {
    int entity_id;
    GFA_Context context;
} Blank3DFactionCandidate;

int blank3d_faction_init(Blank3DFactionSystem *system, const char *ini_path);
int blank3d_faction_register_actor(Blank3DFactionSystem *system,
                                  int entity_id,
                                  const char *faction,
                                  const char *team,
                                  const char *role,
                                  const char *tags_csv,
                                  int threat,
                                  int morale,
                                  int alive,
                                  int targetable);
int blank3d_faction_sync_actor(Blank3DFactionSystem *system,
                              int entity_id,
                              int threat,
                              int morale,
                              int alive,
                              int targetable);
GFA_Decision blank3d_faction_decide(const Blank3DFactionSystem *system,
                                    int source_entity,
                                    int target_entity,
                                    const GFA_Context *context);
int blank3d_faction_can_attack(const Blank3DFactionSystem *system,
                              int source_entity,
                              int target_entity,
                              const GFA_Context *context);
int blank3d_faction_are_allies(const Blank3DFactionSystem *system,
                               int source_entity,
                               int target_entity);
int blank3d_faction_choose_attack_target(
    const Blank3DFactionSystem *system,
    int source_entity,
    const Blank3DFactionCandidate *candidates,
    int candidate_count,
    GFA_Decision *out_decision);
int blank3d_faction_choose_attack_target_stable(
    const Blank3DFactionSystem *system,
    int source_entity,
    const Blank3DFactionCandidate *candidates,
    int candidate_count,
    int current_target,
    int distance_weight,
    int current_target_bonus,
    GFA_Decision *out_decision);
const char *blank3d_faction_status(const Blank3DFactionSystem *system);

#ifdef __cplusplus
}
#endif

#endif
