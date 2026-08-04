#include "gfaction89.h"
#include "gfaction89_tags.h"
#include "gfaction89_matrix.h"
#include <stdio.h>
#include <string.h>

static int gfa_streq(const char *a, const char *b)
{
    if (!a || !b) return GFA_FALSE;
    while (*a && *b) {
        if (*a != *b) return GFA_FALSE;
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0') ? GFA_TRUE : GFA_FALSE;
}

static void gfa_copy_name(char *dst, const char *src)
{
    int i;
    if (!dst) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    for (i = 0; i < GFA_MAX_NAME - 1 && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

static void gfa_init_relation(GFA_Relation *r, int disposition, int priority, int flags)
{
    if (!r) return;
    r->disposition = disposition;
    r->priority = priority;
    r->flags = flags | GFA_FLAG_VALID;
    r->score_bias = 0;
}

static int gfa_register_named(GFA_NamedId *arr, int max_count, int *count, const char *name)
{
    int i;
    if (!arr || !count || !name || !name[0]) return GFA_INVALID_ID;
    for (i = 0; i < max_count; ++i) {
        if (arr[i].used && gfa_streq(arr[i].name, name)) return i;
    }
    for (i = 0; i < max_count; ++i) {
        if (!arr[i].used) {
            arr[i].used = GFA_TRUE;
            gfa_copy_name(arr[i].name, name);
            if (i >= *count) *count = i + 1;
            return i;
        }
    }
    return GFA_INVALID_ID;
}

static int gfa_find_named(const GFA_NamedId *arr, int max_count, const char *name)
{
    int i;
    if (!arr || !name) return GFA_INVALID_ID;
    for (i = 0; i < max_count; ++i) {
        if (arr[i].used && gfa_streq(arr[i].name, name)) return i;
    }
    return GFA_INVALID_ID;
}

static const char *gfa_named_name(const GFA_NamedId *arr, int max_count, int id)
{
    if (!arr) return "?";
    if (id < 0 || id >= max_count) return "?";
    if (!arr[id].used) return "?";
    return arr[id].name;
}

void gfa_init(GFA_World *world)
{
    int i;
    int j;
    if (!world) return;
    memset(world, 0, sizeof(GFA_World));
    world->default_disposition = GFA_DISP_NEUTRAL;
    world->default_priority = 0;
    world->default_flags = 0;
    for (i = 0; i < GFA_MAX_FACTIONS; ++i) {
        for (j = 0; j < GFA_MAX_FACTIONS; ++j) {
            gfa_init_relation(&world->matrix[i][j], GFA_DISP_NEUTRAL, 0, 0);
        }
    }
}

void gfa_set_scratch_arena(GFA_World *world, unsigned char *data, int capacity)
{
    if (!world) return;
    world->scratch.data = data;
    world->scratch.capacity = capacity;
    world->scratch.used = 0;
}

void *gfa_arena_push(GFA_Arena *arena, int bytes)
{
    void *p;
    int aligned;
    if (!arena || !arena->data || bytes <= 0) return 0;
    aligned = (bytes + 3) & ~3;
    if (arena->used + aligned > arena->capacity) return 0;
    p = (void *)(arena->data + arena->used);
    arena->used += aligned;
    return p;
}

void gfa_arena_reset(GFA_Arena *arena)
{
    if (!arena) return;
    arena->used = 0;
}

int gfa_register_faction(GFA_World *world, const char *name)
{
    if (!world) return GFA_INVALID_ID;
    return gfa_register_named(world->factions, GFA_MAX_FACTIONS, &world->faction_count, name);
}

int gfa_register_team(GFA_World *world, const char *name)
{
    if (!world) return GFA_INVALID_ID;
    return gfa_register_named(world->teams, GFA_MAX_TEAMS, &world->team_count, name);
}

int gfa_register_role(GFA_World *world, const char *name)
{
    if (!world) return GFA_INVALID_ID;
    return gfa_register_named(world->roles, GFA_MAX_ROLES, &world->role_count, name);
}

int gfa_register_tag(GFA_World *world, const char *name)
{
    if (!world) return GFA_INVALID_ID;
    return gfa_register_named(world->tags, GFA_MAX_TAGS, &world->tag_count, name);
}

int gfa_find_faction(const GFA_World *world, const char *name)
{
    if (!world) return GFA_INVALID_ID;
    return gfa_find_named(world->factions, GFA_MAX_FACTIONS, name);
}

int gfa_find_team(const GFA_World *world, const char *name)
{
    if (!world) return GFA_INVALID_ID;
    return gfa_find_named(world->teams, GFA_MAX_TEAMS, name);
}

int gfa_find_role(const GFA_World *world, const char *name)
{
    if (!world) return GFA_INVALID_ID;
    return gfa_find_named(world->roles, GFA_MAX_ROLES, name);
}

int gfa_find_tag(const GFA_World *world, const char *name)
{
    if (!world) return GFA_INVALID_ID;
    return gfa_find_named(world->tags, GFA_MAX_TAGS, name);
}

const char *gfa_faction_name(const GFA_World *world, int id)
{
    if (!world) return "?";
    return gfa_named_name(world->factions, GFA_MAX_FACTIONS, id);
}

const char *gfa_team_name(const GFA_World *world, int id)
{
    if (!world) return "?";
    return gfa_named_name(world->teams, GFA_MAX_TEAMS, id);
}

const char *gfa_role_name(const GFA_World *world, int id)
{
    if (!world) return "?";
    return gfa_named_name(world->roles, GFA_MAX_ROLES, id);
}

const char *gfa_tag_name(const GFA_World *world, int id)
{
    if (!world) return "?";
    return gfa_named_name(world->tags, GFA_MAX_TAGS, id);
}

const char *gfa_disposition_name(int disposition)
{
    switch (disposition) {
    case GFA_DISP_IGNORE: return "ignore";
    case GFA_DISP_NEUTRAL: return "neutral";
    case GFA_DISP_ALLY: return "ally";
    case GFA_DISP_FRIENDLY: return "friendly";
    case GFA_DISP_HATE: return "hate";
    case GFA_DISP_FEAR: return "fear";
    case GFA_DISP_PREY: return "prey";
    case GFA_DISP_PROTECT: return "protect";
    case GFA_DISP_RIVAL: return "rival";
    case GFA_DISP_CONTAIN: return "contain";
    case GFA_DISP_AVOID: return "avoid";
    case GFA_DISP_OWNER: return "owner";
    case GFA_DISP_SCRIPTED: return "scripted";
    default: break;
    }
    return "unknown";
}

int gfa_parse_disposition(const char *text)
{
    if (!text) return GFA_DISP_NEUTRAL;
    if (gfa_streq(text, "ignore")) return GFA_DISP_IGNORE;
    if (gfa_streq(text, "neutral")) return GFA_DISP_NEUTRAL;
    if (gfa_streq(text, "ally")) return GFA_DISP_ALLY;
    if (gfa_streq(text, "friendly")) return GFA_DISP_FRIENDLY;
    if (gfa_streq(text, "hate")) return GFA_DISP_HATE;
    if (gfa_streq(text, "enemy")) return GFA_DISP_HATE;
    if (gfa_streq(text, "hostile")) return GFA_DISP_HATE;
    if (gfa_streq(text, "fear")) return GFA_DISP_FEAR;
    if (gfa_streq(text, "prey")) return GFA_DISP_PREY;
    if (gfa_streq(text, "protect")) return GFA_DISP_PROTECT;
    if (gfa_streq(text, "rival")) return GFA_DISP_RIVAL;
    if (gfa_streq(text, "contain")) return GFA_DISP_CONTAIN;
    if (gfa_streq(text, "avoid")) return GFA_DISP_AVOID;
    if (gfa_streq(text, "owner")) return GFA_DISP_OWNER;
    if (gfa_streq(text, "scripted")) return GFA_DISP_SCRIPTED;
    return GFA_DISP_NEUTRAL;
}

static int gfa_token_eq(const char *token, const char *name)
{
    return gfa_streq(token, name);
}

int gfa_parse_flags(const char *text)
{
    int flags;
    char token[GFA_MAX_NAME];
    int ti;
    const char *p;
    char c;
    flags = 0;
    ti = 0;
    p = text;
    if (!text) return 0;
    while (1) {
        c = *p;
        if (c == '|' || c == ',' || c == '+' || c == ' ' || c == '\t' || c == '\0') {
            if (ti > 0) {
                token[ti] = '\0';
                if (gfa_token_eq(token, "attack")) flags |= GFA_FLAG_CAN_ATTACK;
                else if (gfa_token_eq(token, "assist")) flags |= GFA_FLAG_CAN_ASSIST;
                else if (gfa_token_eq(token, "flee")) flags |= GFA_FLAG_CAN_FLEE;
                else if (gfa_token_eq(token, "protect")) flags |= GFA_FLAG_CAN_PROTECT;
                else if (gfa_token_eq(token, "friendly_fire")) flags |= GFA_FLAG_FRIENDLY_FIRE;
                else if (gfa_token_eq(token, "ignore")) flags |= GFA_FLAG_CAN_IGNORE;
                else if (gfa_token_eq(token, "follow")) flags |= GFA_FLAG_CAN_FOLLOW;
                else if (gfa_token_eq(token, "contain")) flags |= GFA_FLAG_CAN_CONTAIN;
                else if (gfa_token_eq(token, "call_help")) flags |= GFA_FLAG_CAN_CALL_HELP;
                else if (gfa_token_eq(token, "convert")) flags |= GFA_FLAG_CAN_CONVERT;
                else if (gfa_token_eq(token, "scripted")) flags |= GFA_FLAG_SCRIPTED;
                ti = 0;
            }
            if (c == '\0') break;
        } else {
            if (ti < GFA_MAX_NAME - 1) {
                token[ti] = c;
                ++ti;
            }
        }
        ++p;
    }
    return flags;
}

int gfa_ensure_entity(GFA_World *world, int entity_id)
{
    int i;
    if (!world) return GFA_FALSE;
    if (entity_id < 0) return GFA_FALSE;
    for (i = 0; i < GFA_MAX_ENTITIES; ++i) {
        if (world->entities[i].used && world->entities[i].entity_id == entity_id) return GFA_TRUE;
    }
    for (i = 0; i < GFA_MAX_ENTITIES; ++i) {
        if (!world->entities[i].used) {
            world->entities[i].used = GFA_TRUE;
            world->entities[i].entity_id = entity_id;
            world->entities[i].faction_id = GFA_INVALID_ID;
            world->entities[i].team_id = GFA_INVALID_ID;
            world->entities[i].role_id = GFA_INVALID_ID;
            world->entities[i].threat = 0;
            world->entities[i].morale = 0;
            world->entities[i].can_be_targeted = GFA_TRUE;
            world->entities[i].alive = GFA_TRUE;
            gfa_tagmask_clear(&world->entities[i].tags);
            world->entity_count++;
            return GFA_TRUE;
        }
    }
    return GFA_FALSE;
}

GFA_Entity *gfa_get_entity(GFA_World *world, int entity_id)
{
    int i;
    if (!world) return 0;
    for (i = 0; i < GFA_MAX_ENTITIES; ++i) {
        if (world->entities[i].used && world->entities[i].entity_id == entity_id) return &world->entities[i];
    }
    return 0;
}

const GFA_Entity *gfa_get_entity_const(const GFA_World *world, int entity_id)
{
    int i;
    if (!world) return 0;
    for (i = 0; i < GFA_MAX_ENTITIES; ++i) {
        if (world->entities[i].used && world->entities[i].entity_id == entity_id) return &world->entities[i];
    }
    return 0;
}

static void gfa_notify_faction_changed(GFA_World *world, int entity_id, int old_faction, int new_faction)
{
    int i;
    if (!world) return;
    for (i = 0; i < GFA_MAX_HOOKS; ++i) {
        if (world->hooks[i].on_faction_changed) {
            world->hooks[i].on_faction_changed(world, entity_id, old_faction, new_faction, world->hooks[i].user);
        }
    }
}

int gfa_set_entity_faction(GFA_World *world, int entity_id, int faction_id)
{
    GFA_Entity *e;
    int old_faction;
    if (!world) return GFA_FALSE;
    if (!gfa_ensure_entity(world, entity_id)) return GFA_FALSE;
    e = gfa_get_entity(world, entity_id);
    if (!e) return GFA_FALSE;
    old_faction = e->faction_id;
    e->faction_id = faction_id;
    if (old_faction != faction_id) gfa_notify_faction_changed(world, entity_id, old_faction, faction_id);
    return GFA_TRUE;
}

int gfa_set_entity_team(GFA_World *world, int entity_id, int team_id)
{
    GFA_Entity *e;
    if (!world) return GFA_FALSE;
    if (!gfa_ensure_entity(world, entity_id)) return GFA_FALSE;
    e = gfa_get_entity(world, entity_id);
    if (!e) return GFA_FALSE;
    e->team_id = team_id;
    return GFA_TRUE;
}

int gfa_set_entity_role(GFA_World *world, int entity_id, int role_id)
{
    GFA_Entity *e;
    if (!world) return GFA_FALSE;
    if (!gfa_ensure_entity(world, entity_id)) return GFA_FALSE;
    e = gfa_get_entity(world, entity_id);
    if (!e) return GFA_FALSE;
    e->role_id = role_id;
    return GFA_TRUE;
}

int gfa_add_entity_tag(GFA_World *world, int entity_id, int tag_id)
{
    GFA_Entity *e;
    if (!world) return GFA_FALSE;
    if (!gfa_ensure_entity(world, entity_id)) return GFA_FALSE;
    e = gfa_get_entity(world, entity_id);
    if (!e) return GFA_FALSE;
    return gfa_tagmask_add(&e->tags, tag_id);
}

int gfa_clear_entity_tag(GFA_World *world, int entity_id, int tag_id)
{
    GFA_Entity *e;
    if (!world) return GFA_FALSE;
    e = gfa_get_entity(world, entity_id);
    if (!e) return GFA_FALSE;
    return gfa_tagmask_remove(&e->tags, tag_id);
}

int gfa_entity_has_tag(const GFA_World *world, int entity_id, int tag_id)
{
    const GFA_Entity *e;
    if (!world) return GFA_FALSE;
    e = gfa_get_entity_const(world, entity_id);
    if (!e) return GFA_FALSE;
    return gfa_tagmask_has(&e->tags, tag_id);
}

int gfa_set_entity_targetable(GFA_World *world, int entity_id, int can_be_targeted)
{
    GFA_Entity *e;
    if (!world) return GFA_FALSE;
    if (!gfa_ensure_entity(world, entity_id)) return GFA_FALSE;
    e = gfa_get_entity(world, entity_id);
    if (!e) return GFA_FALSE;
    e->can_be_targeted = can_be_targeted ? GFA_TRUE : GFA_FALSE;
    return GFA_TRUE;
}

int gfa_set_entity_alive(GFA_World *world, int entity_id, int alive)
{
    GFA_Entity *e;
    if (!world) return GFA_FALSE;
    if (!gfa_ensure_entity(world, entity_id)) return GFA_FALSE;
    e = gfa_get_entity(world, entity_id);
    if (!e) return GFA_FALSE;
    e->alive = alive ? GFA_TRUE : GFA_FALSE;
    return GFA_TRUE;
}

int gfa_set_entity_stats(GFA_World *world, int entity_id, int threat, int morale)
{
    GFA_Entity *e;
    if (!world) return GFA_FALSE;
    if (!gfa_ensure_entity(world, entity_id)) return GFA_FALSE;
    e = gfa_get_entity(world, entity_id);
    if (!e) return GFA_FALSE;
    e->threat = threat;
    e->morale = morale;
    return GFA_TRUE;
}

int gfa_set_relation(GFA_World *world, int src_faction, int dst_faction, int disposition, int priority, int flags, int score_bias)
{
    if (!world) return GFA_FALSE;
    if (src_faction < 0 || src_faction >= GFA_MAX_FACTIONS) return GFA_FALSE;
    if (dst_faction < 0 || dst_faction >= GFA_MAX_FACTIONS) return GFA_FALSE;
    if (!flags) flags = gfa_relation_flags_from_disposition(disposition);
    world->matrix[src_faction][dst_faction] = gfa_make_relation(disposition, priority, flags, score_bias);
    return GFA_TRUE;
}

int gfa_add_tag_rule(GFA_World *world, int src_tag, int dst_tag, int disposition, int priority, int flags, int score_bias)
{
    int i;
    if (!world) return GFA_FALSE;
    for (i = 0; i < GFA_MAX_TAG_RULES; ++i) {
        if (!world->tag_rules[i].used) {
            world->tag_rules[i].used = GFA_TRUE;
            world->tag_rules[i].src_tag = src_tag;
            world->tag_rules[i].dst_tag = dst_tag;
            world->tag_rules[i].disposition = disposition;
            world->tag_rules[i].priority = priority;
            world->tag_rules[i].flags = flags ? flags : gfa_relation_flags_from_disposition(disposition);
            world->tag_rules[i].score_bias = score_bias;
            return GFA_TRUE;
        }
    }
    return GFA_FALSE;
}

int gfa_add_role_rule(GFA_World *world, int src_role, int dst_role, int disposition, int priority, int flags, int score_bias)
{
    int i;
    if (!world) return GFA_FALSE;
    for (i = 0; i < GFA_MAX_ROLE_RULES; ++i) {
        if (!world->role_rules[i].used) {
            world->role_rules[i].used = GFA_TRUE;
            world->role_rules[i].src_role = src_role;
            world->role_rules[i].dst_role = dst_role;
            world->role_rules[i].disposition = disposition;
            world->role_rules[i].priority = priority;
            world->role_rules[i].flags = flags ? flags : gfa_relation_flags_from_disposition(disposition);
            world->role_rules[i].score_bias = score_bias;
            return GFA_TRUE;
        }
    }
    return GFA_FALSE;
}

int gfa_add_override(GFA_World *world, int src_entity, int dst_entity, int disposition, int priority, int flags, int score_bias)
{
    int i;
    if (!world) return GFA_FALSE;
    for (i = 0; i < GFA_MAX_OVERRIDES; ++i) {
        if (!world->overrides[i].used) {
            world->overrides[i].used = GFA_TRUE;
            world->overrides[i].src_entity = src_entity;
            world->overrides[i].dst_entity = dst_entity;
            world->overrides[i].disposition = disposition;
            world->overrides[i].priority = priority;
            world->overrides[i].flags = flags ? flags : gfa_relation_flags_from_disposition(disposition);
            world->overrides[i].score_bias = score_bias;
            return GFA_TRUE;
        }
    }
    return GFA_FALSE;
}

int gfa_clear_override(GFA_World *world, int src_entity, int dst_entity)
{
    int i;
    int removed;
    if (!world) return GFA_FALSE;
    removed = GFA_FALSE;
    for (i = 0; i < GFA_MAX_OVERRIDES; ++i) {
        if (world->overrides[i].used && world->overrides[i].src_entity == src_entity && world->overrides[i].dst_entity == dst_entity) {
            world->overrides[i].used = GFA_FALSE;
            removed = GFA_TRUE;
        }
    }
    return removed;
}

static void gfa_apply_rel(GFA_Decision *d, int disposition, int priority, int flags, int score_bias, int source_kind)
{
    if (!d) return;
    d->disposition = disposition;
    d->priority = priority;
    d->flags = flags | GFA_FLAG_VALID;
    d->source_kind = source_kind;
    d->score_fp = GFA_FP_FROM_INT(priority + gfa_relation_base_score(disposition) + score_bias);
}

static int gfa_match_tag_rule(const GFA_Entity *src, const GFA_Entity *dst, const GFA_TagRule *rule)
{
    int src_ok;
    int dst_ok;
    if (!src || !dst || !rule || !rule->used) return GFA_FALSE;
    src_ok = (rule->src_tag == GFA_MATCH_ANY) ? GFA_TRUE : gfa_tagmask_has(&src->tags, rule->src_tag);
    dst_ok = (rule->dst_tag == GFA_MATCH_ANY) ? GFA_TRUE : gfa_tagmask_has(&dst->tags, rule->dst_tag);
    return (src_ok && dst_ok) ? GFA_TRUE : GFA_FALSE;
}

static int gfa_match_role_rule(const GFA_Entity *src, const GFA_Entity *dst, const GFA_RoleRule *rule)
{
    int src_ok;
    int dst_ok;
    if (!src || !dst || !rule || !rule->used) return GFA_FALSE;
    src_ok = (rule->src_role == GFA_MATCH_ANY || rule->src_role == src->role_id) ? GFA_TRUE : GFA_FALSE;
    dst_ok = (rule->dst_role == GFA_MATCH_ANY || rule->dst_role == dst->role_id) ? GFA_TRUE : GFA_FALSE;
    return (src_ok && dst_ok) ? GFA_TRUE : GFA_FALSE;
}

static void gfa_finalize_decision(GFA_Decision *d, const GFA_Entity *dst, const GFA_Context *ctx)
{
    int flags;
    int score;
    if (!d) return;
    flags = d->flags;
    d->can_attack = (flags & GFA_FLAG_CAN_ATTACK) ? GFA_TRUE : GFA_FALSE;
    d->can_assist = (flags & GFA_FLAG_CAN_ASSIST) ? GFA_TRUE : GFA_FALSE;
    d->can_flee_from = (flags & GFA_FLAG_CAN_FLEE) ? GFA_TRUE : GFA_FALSE;
    d->can_protect = (flags & GFA_FLAG_CAN_PROTECT) ? GFA_TRUE : GFA_FALSE;
    d->can_follow = (flags & GFA_FLAG_CAN_FOLLOW) ? GFA_TRUE : GFA_FALSE;
    d->can_call_help = (flags & GFA_FLAG_CAN_CALL_HELP) ? GFA_TRUE : GFA_FALSE;

    score = d->score_fp;
    if (dst) score += GFA_FP_FROM_INT(dst->threat / 2);
    if (ctx) {
        score += GFA_FP_FROM_INT(ctx->mission_bias);
        if (ctx->recent_damage) score += GFA_FP_FROM_INT(40);
        if (ctx->visible) score += GFA_FP_FROM_INT(12);
        if (ctx->heard) score += GFA_FP_FROM_INT(4);
        if (ctx->distance_fp > 0) score -= ctx->distance_fp / 16;
    }
    d->score_fp = score;
}

GFA_Decision gfa_eval(const GFA_World *world, int src_entity, int dst_entity, const GFA_Context *ctx)
{
    GFA_Decision d;
    const GFA_Entity *src;
    const GFA_Entity *dst;
    GFA_Relation rel;
    int i;
    int best_priority;

    memset(&d, 0, sizeof(GFA_Decision));
    d.disposition = GFA_DISP_NEUTRAL;
    d.priority = 0;
    d.flags = 0;
    d.score_fp = 0;
    d.source_kind = 0;

    if (!world) return d;
    src = gfa_get_entity_const(world, src_entity);
    dst = gfa_get_entity_const(world, dst_entity);
    if (!src || !dst) return d;
    if (!src->alive || !dst->alive || !dst->can_be_targeted) {
        gfa_apply_rel(&d, GFA_DISP_IGNORE, 999, GFA_FLAG_CAN_IGNORE, 0, 1);
        gfa_finalize_decision(&d, dst, ctx);
        return d;
    }

    gfa_apply_rel(&d, world->default_disposition, world->default_priority, world->default_flags, 0, 1);
    best_priority = world->default_priority;

    if (src->faction_id >= 0 && src->faction_id < GFA_MAX_FACTIONS && dst->faction_id >= 0 && dst->faction_id < GFA_MAX_FACTIONS) {
        rel = world->matrix[src->faction_id][dst->faction_id];
        if (rel.flags & GFA_FLAG_VALID) {
            gfa_apply_rel(&d, rel.disposition, rel.priority, rel.flags, rel.score_bias, 2);
            best_priority = rel.priority;
        }
    }

    if (src->team_id >= 0 && dst->team_id >= 0 && src->team_id == dst->team_id) {
        if (best_priority <= 20) {
            gfa_apply_rel(&d, GFA_DISP_ALLY, 20, GFA_FLAG_CAN_ASSIST | GFA_FLAG_CAN_FOLLOW, 0, 3);
            best_priority = 20;
        }
    }

    for (i = 0; i < GFA_MAX_ROLE_RULES; ++i) {
        if (gfa_match_role_rule(src, dst, &world->role_rules[i])) {
            if (world->role_rules[i].priority >= best_priority) {
                gfa_apply_rel(&d, world->role_rules[i].disposition, world->role_rules[i].priority, world->role_rules[i].flags, world->role_rules[i].score_bias, 4);
                best_priority = world->role_rules[i].priority;
            }
        }
    }

    for (i = 0; i < GFA_MAX_TAG_RULES; ++i) {
        if (gfa_match_tag_rule(src, dst, &world->tag_rules[i])) {
            if (world->tag_rules[i].priority >= best_priority) {
                gfa_apply_rel(&d, world->tag_rules[i].disposition, world->tag_rules[i].priority, world->tag_rules[i].flags, world->tag_rules[i].score_bias, 5);
                best_priority = world->tag_rules[i].priority;
            }
        }
    }

    for (i = 0; i < GFA_MAX_OVERRIDES; ++i) {
        if (world->overrides[i].used && world->overrides[i].src_entity == src_entity && world->overrides[i].dst_entity == dst_entity) {
            if (world->overrides[i].priority >= best_priority) {
                gfa_apply_rel(&d, world->overrides[i].disposition, world->overrides[i].priority, world->overrides[i].flags, world->overrides[i].score_bias, 6);
                best_priority = world->overrides[i].priority;
            }
        }
    }

    gfa_finalize_decision(&d, dst, ctx);
    return d;
}

int gfa_choose_best_target(const GFA_World *world, int src_entity, const int *candidates, int candidate_count, const GFA_Context *ctx, GFA_Decision *out_decision)
{
    int i;
    int best;
    int best_score;
    GFA_Decision d;
    if (!world || !candidates || candidate_count <= 0) return GFA_INVALID_ID;
    best = GFA_INVALID_ID;
    best_score = -2147480000;
    for (i = 0; i < candidate_count; ++i) {
        d = gfa_eval(world, src_entity, candidates[i], ctx);
        if (d.can_attack) {
            if (d.score_fp > best_score) {
                best_score = d.score_fp;
                best = candidates[i];
                if (out_decision) *out_decision = d;
            }
        }
    }
    return best;
}

int gfa_filter_candidates(const GFA_World *world, int src_entity, const int *candidates, int candidate_count, int *out_candidates, int out_max, int required_flags, const GFA_Context *ctx)
{
    int i;
    int count;
    GFA_Decision d;
    if (!world || !candidates || !out_candidates || candidate_count <= 0 || out_max <= 0) return 0;
    count = 0;
    for (i = 0; i < candidate_count; ++i) {
        d = gfa_eval(world, src_entity, candidates[i], ctx);
        if ((d.flags & required_flags) == required_flags) {
            out_candidates[count] = candidates[i];
            ++count;
            if (count >= out_max) break;
        }
    }
    return count;
}

int gfa_add_trigger(GFA_World *world, const GFA_Trigger *trigger_data)
{
    int i;
    if (!world || !trigger_data) return GFA_FALSE;
    for (i = 0; i < GFA_MAX_TRIGGERS; ++i) {
        if (!world->triggers[i].used) {
            world->triggers[i] = *trigger_data;
            world->triggers[i].used = GFA_TRUE;
            return GFA_TRUE;
        }
    }
    return GFA_FALSE;
}

static int gfa_trigger_matches(const GFA_World *world, const GFA_Trigger *t, const GFA_Event *event_data)
{
    const GFA_Entity *src;
    const GFA_Entity *dst;
    GFA_Decision d;
    if (!world || !t || !event_data || !t->used) return GFA_FALSE;
    if (t->event_type != GFA_MATCH_ANY && t->event_type != event_data->event_type) return GFA_FALSE;
    src = gfa_get_entity_const(world, event_data->source_entity);
    dst = gfa_get_entity_const(world, event_data->target_entity);
    if (!src || !dst) return GFA_FALSE;
    if (t->src_faction != GFA_MATCH_ANY && t->src_faction != src->faction_id) return GFA_FALSE;
    if (t->dst_faction != GFA_MATCH_ANY && t->dst_faction != dst->faction_id) return GFA_FALSE;
    if (t->src_team != GFA_MATCH_ANY && t->src_team != src->team_id) return GFA_FALSE;
    if (t->dst_team != GFA_MATCH_ANY && t->dst_team != dst->team_id) return GFA_FALSE;
    if (t->src_role != GFA_MATCH_ANY && t->src_role != src->role_id) return GFA_FALSE;
    if (t->dst_role != GFA_MATCH_ANY && t->dst_role != dst->role_id) return GFA_FALSE;
    if (t->src_tag != GFA_MATCH_ANY && !gfa_tagmask_has(&src->tags, t->src_tag)) return GFA_FALSE;
    if (t->dst_tag != GFA_MATCH_ANY && !gfa_tagmask_has(&dst->tags, t->dst_tag)) return GFA_FALSE;
    if (t->required_disposition != GFA_MATCH_ANY) {
        d = gfa_eval(world, event_data->source_entity, event_data->target_entity, 0);
        if (d.disposition != t->required_disposition) return GFA_FALSE;
    }
    return GFA_TRUE;
}

static void gfa_notify_trigger(GFA_World *world, const GFA_Event *event_data, const GFA_Trigger *trigger_data)
{
    int i;
    if (!world) return;
    for (i = 0; i < GFA_MAX_HOOKS; ++i) {
        if (world->hooks[i].on_trigger) {
            world->hooks[i].on_trigger(world, event_data, trigger_data, world->hooks[i].user);
        }
    }
}

static void gfa_apply_trigger(GFA_World *world, const GFA_Trigger *t, const GFA_Event *event_data)
{
    if (!world || !t || !event_data) return;
    if (t->set_src_faction != GFA_MATCH_ANY) gfa_set_entity_faction(world, event_data->source_entity, t->set_src_faction);
    if (t->set_dst_faction != GFA_MATCH_ANY) gfa_set_entity_faction(world, event_data->target_entity, t->set_dst_faction);
    if (t->set_src_team != GFA_MATCH_ANY) gfa_set_entity_team(world, event_data->source_entity, t->set_src_team);
    if (t->set_dst_team != GFA_MATCH_ANY) gfa_set_entity_team(world, event_data->target_entity, t->set_dst_team);
    if (t->set_src_role != GFA_MATCH_ANY) gfa_set_entity_role(world, event_data->source_entity, t->set_src_role);
    if (t->set_dst_role != GFA_MATCH_ANY) gfa_set_entity_role(world, event_data->target_entity, t->set_dst_role);
    if (t->add_src_tag != GFA_MATCH_ANY) gfa_add_entity_tag(world, event_data->source_entity, t->add_src_tag);
    if (t->add_dst_tag != GFA_MATCH_ANY) gfa_add_entity_tag(world, event_data->target_entity, t->add_dst_tag);
    if (t->clear_src_tag != GFA_MATCH_ANY) gfa_clear_entity_tag(world, event_data->source_entity, t->clear_src_tag);
    if (t->clear_dst_tag != GFA_MATCH_ANY) gfa_clear_entity_tag(world, event_data->target_entity, t->clear_dst_tag);
    if (t->mark_src_alive != GFA_MATCH_ANY) gfa_set_entity_alive(world, event_data->source_entity, t->mark_src_alive);
    if (t->mark_dst_alive != GFA_MATCH_ANY) gfa_set_entity_alive(world, event_data->target_entity, t->mark_dst_alive);
}

int gfa_fire_event(GFA_World *world, const GFA_Event *event_data)
{
    int i;
    int fired;
    if (!world || !event_data) return 0;
    fired = 0;
    for (i = 0; i < GFA_MAX_TRIGGERS; ++i) {
        if (gfa_trigger_matches(world, &world->triggers[i], event_data)) {
            gfa_apply_trigger(world, &world->triggers[i], event_data);
            gfa_notify_trigger(world, event_data, &world->triggers[i]);
            ++fired;
        }
    }
    return fired;
}

int gfa_add_hooks(GFA_World *world, const GFA_Hooks *hooks)
{
    int i;
    if (!world || !hooks) return GFA_FALSE;
    for (i = 0; i < GFA_MAX_HOOKS; ++i) {
        if (!world->hooks[i].on_faction_changed && !world->hooks[i].on_decision && !world->hooks[i].on_trigger) {
            world->hooks[i] = *hooks;
            return GFA_TRUE;
        }
    }
    return GFA_FALSE;
}

void gfa_debug_print_entity(const GFA_World *world, int entity_id)
{
    const GFA_Entity *e;
    int i;
    if (!world) return;
    e = gfa_get_entity_const(world, entity_id);
    if (!e) {
        printf("entity %d: <missing>\n", entity_id);
        return;
    }
    printf("entity %d faction=%s team=%s role=%s alive=%d tags=", entity_id,
        gfa_faction_name(world, e->faction_id), gfa_team_name(world, e->team_id),
        gfa_role_name(world, e->role_id), e->alive);
    for (i = 0; i < GFA_MAX_TAGS; ++i) {
        if (gfa_tagmask_has(&e->tags, i)) printf("%s ", gfa_tag_name(world, i));
    }
    printf("\n");
}

void gfa_debug_print_decision(const GFA_World *world, int src_entity, int dst_entity, const GFA_Decision *decision)
{
    (void)world;
    if (!decision) return;
    printf("%d -> %d disp=%s priority=%d flags=0x%04x score=%d src=%d\n",
        src_entity, dst_entity, gfa_disposition_name(decision->disposition), decision->priority,
        decision->flags, GFA_FP_TO_INT(decision->score_fp), decision->source_kind);
}
