#include "gfaction89_ini.h"
#include "gfaction89_matrix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int ini_streq(const char *a, const char *b)
{
    if (!a || !b) return GFA_FALSE;
    while (*a && *b) {
        if (*a != *b) return GFA_FALSE;
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0') ? GFA_TRUE : GFA_FALSE;
}

static void ini_copy(char *dst, const char *src, int max_len)
{
    int i;
    if (!dst || max_len <= 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    for (i = 0; i < max_len - 1 && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

static void ini_trim_inplace(char *s)
{
    char *p;
    char *e;
    int len;
    if (!s) return;
    p = s;
    while (*p && isspace((unsigned char)*p)) ++p;
    if (p != s) memmove(s, p, strlen(p) + 1);
    len = (int)strlen(s);
    if (len <= 0) return;
    e = s + len - 1;
    while (e >= s && isspace((unsigned char)*e)) {
        *e = '\0';
        --e;
    }
}

static void ini_strip_comment(char *s)
{
    char *p;
    if (!s) return;
    p = s;
    while (*p) {
        if (*p == ';' || *p == '#') {
            *p = '\0';
            return;
        }
        ++p;
    }
}

static int ini_prefix(const char *s, const char *prefix)
{
    while (*prefix) {
        if (*s != *prefix) return GFA_FALSE;
        ++s;
        ++prefix;
    }
    return GFA_TRUE;
}

static int ini_split2(const char *s, char *a, char *b)
{
    const char *p;
    int i;
    if (!s || !a || !b) return GFA_FALSE;
    p = strchr(s, ':');
    if (!p) return GFA_FALSE;
    for (i = 0; s + i < p && i < GFA_MAX_NAME - 1; ++i) a[i] = s[i];
    a[i] = '\0';
    ini_copy(b, p + 1, GFA_MAX_NAME);
    ini_trim_inplace(a);
    ini_trim_inplace(b);
    return GFA_TRUE;
}

int gfa_ini_tokenize_csv(const char *text, char tokens[GFA_MAX_TOKENS][GFA_MAX_NAME], int max_tokens)
{
    int count;
    int ti;
    int ci;
    char c;
    const char *p;
    if (!text || !tokens || max_tokens <= 0) return 0;
    count = 0;
    ti = 0;
    ci = 0;
    p = text;
    while (1) {
        c = *p;
        if (c == ',' || c == '|' || c == '+' || c == ' ' || c == '\t' || c == '\0') {
            if (ci > 0) {
                tokens[count][ci] = '\0';
                ++count;
                ci = 0;
                if (count >= max_tokens) break;
            }
            if (c == '\0') break;
        } else {
            if (ci < GFA_MAX_NAME - 1) {
                tokens[count][ci] = c;
                ++ci;
            }
        }
        ++p;
        ++ti;
        if (ti > 2048) break;
    }
    return count;
}

static int ini_parse_bool(const char *value)
{
    if (!value) return GFA_FALSE;
    if (ini_streq(value, "yes")) return GFA_TRUE;
    if (ini_streq(value, "true")) return GFA_TRUE;
    if (ini_streq(value, "on")) return GFA_TRUE;
    if (ini_streq(value, "1")) return GFA_TRUE;
    return GFA_FALSE;
}

static int ini_parse_int_or_match(const char *value)
{
    if (!value) return GFA_MATCH_ANY;
    if (ini_streq(value, "any")) return GFA_MATCH_ANY;
    if (ini_streq(value, "*")) return GFA_MATCH_ANY;
    return atoi(value);
}

static int ini_find_or_register_faction(GFA_World *world, const char *name)
{
    int id;
    id = gfa_find_faction(world, name);
    if (id == GFA_INVALID_ID) id = gfa_register_faction(world, name);
    return id;
}

static int ini_find_or_register_team(GFA_World *world, const char *name)
{
    int id;
    if (ini_streq(name, "none") || ini_streq(name, "-1")) return GFA_INVALID_ID;
    id = gfa_find_team(world, name);
    if (id == GFA_INVALID_ID) id = gfa_register_team(world, name);
    return id;
}

static int ini_find_or_register_role(GFA_World *world, const char *name)
{
    int id;
    if (ini_streq(name, "none") || ini_streq(name, "-1")) return GFA_INVALID_ID;
    id = gfa_find_role(world, name);
    if (id == GFA_INVALID_ID) id = gfa_register_role(world, name);
    return id;
}

static int ini_find_or_register_tag(GFA_World *world, const char *name)
{
    int id;
    if (ini_streq(name, "any") || ini_streq(name, "*")) return GFA_MATCH_ANY;
    id = gfa_find_tag(world, name);
    if (id == GFA_INVALID_ID) id = gfa_register_tag(world, name);
    return id;
}

static GFA_TagRule *ini_get_tag_rule(GFA_World *world, int src_tag, int dst_tag)
{
    int i;
    if (!world) return 0;
    for (i = 0; i < GFA_MAX_TAG_RULES; ++i) {
        if (world->tag_rules[i].used && world->tag_rules[i].src_tag == src_tag && world->tag_rules[i].dst_tag == dst_tag) return &world->tag_rules[i];
    }
    for (i = 0; i < GFA_MAX_TAG_RULES; ++i) {
        if (!world->tag_rules[i].used) {
            world->tag_rules[i].used = GFA_TRUE;
            world->tag_rules[i].src_tag = src_tag;
            world->tag_rules[i].dst_tag = dst_tag;
            world->tag_rules[i].disposition = GFA_DISP_NEUTRAL;
            world->tag_rules[i].priority = 0;
            world->tag_rules[i].flags = 0;
            world->tag_rules[i].score_bias = 0;
            return &world->tag_rules[i];
        }
    }
    return 0;
}

static GFA_RoleRule *ini_get_role_rule(GFA_World *world, int src_role, int dst_role)
{
    int i;
    if (!world) return 0;
    for (i = 0; i < GFA_MAX_ROLE_RULES; ++i) {
        if (world->role_rules[i].used && world->role_rules[i].src_role == src_role && world->role_rules[i].dst_role == dst_role) return &world->role_rules[i];
    }
    for (i = 0; i < GFA_MAX_ROLE_RULES; ++i) {
        if (!world->role_rules[i].used) {
            world->role_rules[i].used = GFA_TRUE;
            world->role_rules[i].src_role = src_role;
            world->role_rules[i].dst_role = dst_role;
            world->role_rules[i].disposition = GFA_DISP_NEUTRAL;
            world->role_rules[i].priority = 0;
            world->role_rules[i].flags = 0;
            world->role_rules[i].score_bias = 0;
            return &world->role_rules[i];
        }
    }
    return 0;
}

static GFA_Override *ini_get_override(GFA_World *world, int src_entity, int dst_entity)
{
    int i;
    if (!world) return 0;
    for (i = 0; i < GFA_MAX_OVERRIDES; ++i) {
        if (world->overrides[i].used && world->overrides[i].src_entity == src_entity && world->overrides[i].dst_entity == dst_entity) return &world->overrides[i];
    }
    for (i = 0; i < GFA_MAX_OVERRIDES; ++i) {
        if (!world->overrides[i].used) {
            world->overrides[i].used = GFA_TRUE;
            world->overrides[i].src_entity = src_entity;
            world->overrides[i].dst_entity = dst_entity;
            world->overrides[i].disposition = GFA_DISP_NEUTRAL;
            world->overrides[i].priority = 0;
            world->overrides[i].flags = 0;
            world->overrides[i].score_bias = 0;
            return &world->overrides[i];
        }
    }
    return 0;
}

static void ini_trigger_defaults(GFA_Trigger *t)
{
    if (!t) return;
    memset(t, 0, sizeof(GFA_Trigger));
    t->used = GFA_TRUE;
    t->event_type = GFA_MATCH_ANY;
    t->src_faction = GFA_MATCH_ANY;
    t->dst_faction = GFA_MATCH_ANY;
    t->src_team = GFA_MATCH_ANY;
    t->dst_team = GFA_MATCH_ANY;
    t->src_role = GFA_MATCH_ANY;
    t->dst_role = GFA_MATCH_ANY;
    t->src_tag = GFA_MATCH_ANY;
    t->dst_tag = GFA_MATCH_ANY;
    t->required_disposition = GFA_MATCH_ANY;
    t->set_src_faction = GFA_MATCH_ANY;
    t->set_dst_faction = GFA_MATCH_ANY;
    t->set_src_team = GFA_MATCH_ANY;
    t->set_dst_team = GFA_MATCH_ANY;
    t->set_src_role = GFA_MATCH_ANY;
    t->set_dst_role = GFA_MATCH_ANY;
    t->add_src_tag = GFA_MATCH_ANY;
    t->add_dst_tag = GFA_MATCH_ANY;
    t->clear_src_tag = GFA_MATCH_ANY;
    t->clear_dst_tag = GFA_MATCH_ANY;
    t->mark_src_alive = GFA_MATCH_ANY;
    t->mark_dst_alive = GFA_MATCH_ANY;
}

static GFA_Trigger *ini_get_trigger(GFA_World *world, const char *name)
{
    int i;
    if (!world || !name) return 0;
    for (i = 0; i < GFA_MAX_TRIGGERS; ++i) {
        if (world->triggers[i].used && ini_streq(world->triggers[i].name, name)) return &world->triggers[i];
    }
    for (i = 0; i < GFA_MAX_TRIGGERS; ++i) {
        if (!world->triggers[i].used) {
            ini_trigger_defaults(&world->triggers[i]);
            ini_copy(world->triggers[i].name, name, GFA_MAX_NAME);
            return &world->triggers[i];
        }
    }
    return 0;
}

static int ini_event_type(const char *value)
{
    if (ini_streq(value, "death")) return GFA_EVENT_DEATH;
    if (ini_streq(value, "damage")) return GFA_EVENT_DAMAGE;
    if (ini_streq(value, "sense")) return GFA_EVENT_SENSE;
    if (ini_streq(value, "touch")) return GFA_EVENT_TOUCH;
    if (ini_streq(value, "script")) return GFA_EVENT_SCRIPT;
    if (ini_streq(value, "any")) return GFA_MATCH_ANY;
    if (ini_streq(value, "*")) return GFA_MATCH_ANY;
    return atoi(value);
}

static void ini_apply_relation(GFA_World *world, const char *tail, const char *key, const char *value)
{
    char a[GFA_MAX_NAME];
    char b[GFA_MAX_NAME];
    int src;
    int dst;
    GFA_Relation r;
    if (!ini_split2(tail, a, b)) return;
    src = ini_find_or_register_faction(world, a);
    dst = ini_find_or_register_faction(world, b);
    if (src < 0 || dst < 0) return;
    r = world->matrix[src][dst];
    if (ini_streq(key, "disposition")) {
        r.disposition = gfa_parse_disposition(value);
        if (!(r.flags & ~GFA_FLAG_VALID)) r.flags = gfa_relation_flags_from_disposition(r.disposition) | GFA_FLAG_VALID;
    } else if (ini_streq(key, "priority")) {
        r.priority = atoi(value);
    } else if (ini_streq(key, "flags")) {
        r.flags = gfa_parse_flags(value) | GFA_FLAG_VALID;
    } else if (ini_streq(key, "score_bias")) {
        r.score_bias = atoi(value);
    }
    world->matrix[src][dst] = r;
}

static void ini_apply_tag_rule(GFA_World *world, const char *tail, const char *key, const char *value)
{
    char a[GFA_MAX_NAME];
    char b[GFA_MAX_NAME];
    int src;
    int dst;
    GFA_TagRule *r;
    if (!ini_split2(tail, a, b)) return;
    src = ini_find_or_register_tag(world, a);
    dst = ini_find_or_register_tag(world, b);
    r = ini_get_tag_rule(world, src, dst);
    if (!r) return;
    if (ini_streq(key, "disposition")) {
        r->disposition = gfa_parse_disposition(value);
        if (!r->flags) r->flags = gfa_relation_flags_from_disposition(r->disposition);
    } else if (ini_streq(key, "priority")) r->priority = atoi(value);
    else if (ini_streq(key, "flags")) r->flags = gfa_parse_flags(value);
    else if (ini_streq(key, "score_bias")) r->score_bias = atoi(value);
}

static void ini_apply_role_rule(GFA_World *world, const char *tail, const char *key, const char *value)
{
    char a[GFA_MAX_NAME];
    char b[GFA_MAX_NAME];
    int src;
    int dst;
    GFA_RoleRule *r;
    if (!ini_split2(tail, a, b)) return;
    src = ini_find_or_register_role(world, a);
    dst = ini_find_or_register_role(world, b);
    r = ini_get_role_rule(world, src, dst);
    if (!r) return;
    if (ini_streq(key, "disposition")) {
        r->disposition = gfa_parse_disposition(value);
        if (!r->flags) r->flags = gfa_relation_flags_from_disposition(r->disposition);
    } else if (ini_streq(key, "priority")) r->priority = atoi(value);
    else if (ini_streq(key, "flags")) r->flags = gfa_parse_flags(value);
    else if (ini_streq(key, "score_bias")) r->score_bias = atoi(value);
}

static void ini_apply_override(GFA_World *world, const char *tail, const char *key, const char *value)
{
    char a[GFA_MAX_NAME];
    char b[GFA_MAX_NAME];
    int src;
    int dst;
    GFA_Override *r;
    if (!ini_split2(tail, a, b)) return;
    src = atoi(a);
    dst = atoi(b);
    r = ini_get_override(world, src, dst);
    if (!r) return;
    if (ini_streq(key, "disposition")) {
        r->disposition = gfa_parse_disposition(value);
        if (!r->flags) r->flags = gfa_relation_flags_from_disposition(r->disposition);
    } else if (ini_streq(key, "priority")) r->priority = atoi(value);
    else if (ini_streq(key, "flags")) r->flags = gfa_parse_flags(value);
    else if (ini_streq(key, "score_bias")) r->score_bias = atoi(value);
}

static void ini_apply_entity(GFA_World *world, const char *tail, const char *key, const char *value)
{
    int id;
    int i;
    int count;
    int tag_id;
    char tokens[GFA_MAX_TOKENS][GFA_MAX_NAME];
    id = atoi(tail);
    gfa_ensure_entity(world, id);
    if (ini_streq(key, "faction")) gfa_set_entity_faction(world, id, ini_find_or_register_faction(world, value));
    else if (ini_streq(key, "team")) gfa_set_entity_team(world, id, ini_find_or_register_team(world, value));
    else if (ini_streq(key, "role")) gfa_set_entity_role(world, id, ini_find_or_register_role(world, value));
    else if (ini_streq(key, "tags")) {
        count = gfa_ini_tokenize_csv(value, tokens, GFA_MAX_TOKENS);
        for (i = 0; i < count; ++i) {
            tag_id = ini_find_or_register_tag(world, tokens[i]);
            if (tag_id >= 0) gfa_add_entity_tag(world, id, tag_id);
        }
    } else if (ini_streq(key, "threat")) {
        GFA_Entity *e;
        e = gfa_get_entity(world, id);
        if (e) e->threat = atoi(value);
    } else if (ini_streq(key, "morale")) {
        GFA_Entity *e;
        e = gfa_get_entity(world, id);
        if (e) e->morale = atoi(value);
    } else if (ini_streq(key, "alive")) gfa_set_entity_alive(world, id, ini_parse_bool(value));
    else if (ini_streq(key, "targetable")) gfa_set_entity_targetable(world, id, ini_parse_bool(value));
}

static void ini_apply_trigger(GFA_World *world, const char *tail, const char *key, const char *value)
{
    GFA_Trigger *t;
    t = ini_get_trigger(world, tail);
    if (!t) return;
    if (ini_streq(key, "event")) t->event_type = ini_event_type(value);
    else if (ini_streq(key, "src_faction")) t->src_faction = ini_find_or_register_faction(world, value);
    else if (ini_streq(key, "dst_faction")) t->dst_faction = ini_find_or_register_faction(world, value);
    else if (ini_streq(key, "src_team")) t->src_team = ini_find_or_register_team(world, value);
    else if (ini_streq(key, "dst_team")) t->dst_team = ini_find_or_register_team(world, value);
    else if (ini_streq(key, "src_role")) t->src_role = ini_find_or_register_role(world, value);
    else if (ini_streq(key, "dst_role")) t->dst_role = ini_find_or_register_role(world, value);
    else if (ini_streq(key, "src_tag")) t->src_tag = ini_find_or_register_tag(world, value);
    else if (ini_streq(key, "dst_tag")) t->dst_tag = ini_find_or_register_tag(world, value);
    else if (ini_streq(key, "required_disposition")) t->required_disposition = gfa_parse_disposition(value);
    else if (ini_streq(key, "set_src_faction")) t->set_src_faction = ini_find_or_register_faction(world, value);
    else if (ini_streq(key, "set_dst_faction")) t->set_dst_faction = ini_find_or_register_faction(world, value);
    else if (ini_streq(key, "set_src_team")) t->set_src_team = ini_find_or_register_team(world, value);
    else if (ini_streq(key, "set_dst_team")) t->set_dst_team = ini_find_or_register_team(world, value);
    else if (ini_streq(key, "set_src_role")) t->set_src_role = ini_find_or_register_role(world, value);
    else if (ini_streq(key, "set_dst_role")) t->set_dst_role = ini_find_or_register_role(world, value);
    else if (ini_streq(key, "add_src_tag")) t->add_src_tag = ini_find_or_register_tag(world, value);
    else if (ini_streq(key, "add_dst_tag")) t->add_dst_tag = ini_find_or_register_tag(world, value);
    else if (ini_streq(key, "clear_src_tag")) t->clear_src_tag = ini_find_or_register_tag(world, value);
    else if (ini_streq(key, "clear_dst_tag")) t->clear_dst_tag = ini_find_or_register_tag(world, value);
    else if (ini_streq(key, "mark_src_alive")) t->mark_src_alive = ini_parse_int_or_match(value);
    else if (ini_streq(key, "mark_dst_alive")) t->mark_dst_alive = ini_parse_int_or_match(value);
}

int gfa_ini_apply_pair(GFA_World *world, const char *section, const char *key, const char *value)
{
    if (!world || !section || !key || !value) return GFA_FALSE;
    if (ini_streq(section, "factions")) {
        gfa_register_faction(world, value);
        return GFA_TRUE;
    }
    if (ini_streq(section, "teams")) {
        gfa_register_team(world, value);
        return GFA_TRUE;
    }
    if (ini_streq(section, "roles")) {
        gfa_register_role(world, value);
        return GFA_TRUE;
    }
    if (ini_streq(section, "tags")) {
        gfa_register_tag(world, value);
        return GFA_TRUE;
    }
    if (ini_streq(section, "defaults")) {
        if (ini_streq(key, "disposition")) world->default_disposition = gfa_parse_disposition(value);
        else if (ini_streq(key, "priority")) world->default_priority = atoi(value);
        else if (ini_streq(key, "flags")) world->default_flags = gfa_parse_flags(value);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "faction:")) {
        gfa_register_faction(world, section + 8);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "team:")) {
        gfa_register_team(world, section + 5);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "role:")) {
        gfa_register_role(world, section + 5);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "tag:")) {
        gfa_register_tag(world, section + 4);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "entity:")) {
        ini_apply_entity(world, section + 7, key, value);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "relation:")) {
        ini_apply_relation(world, section + 9, key, value);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "tag_rule:")) {
        ini_apply_tag_rule(world, section + 9, key, value);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "role_rule:")) {
        ini_apply_role_rule(world, section + 10, key, value);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "override:")) {
        ini_apply_override(world, section + 9, key, value);
        return GFA_TRUE;
    }
    if (ini_prefix(section, "trigger:")) {
        ini_apply_trigger(world, section + 8, key, value);
        return GFA_TRUE;
    }
    return GFA_FALSE;
}

int gfa_load_ini_text(GFA_World *world, const char *text)
{
    char line[512];
    char section[128];
    char key[128];
    char value[256];
    const char *p;
    int li;
    char *eq;
    int applied;
    if (!world || !text) return GFA_FALSE;
    section[0] = '\0';
    p = text;
    applied = 0;
    while (*p) {
        li = 0;
        while (*p && *p != '\n' && li < (int)sizeof(line) - 1) {
            line[li] = *p;
            ++li;
            ++p;
        }
        while (*p && *p != '\n') ++p;
        if (*p == '\n') ++p;
        line[li] = '\0';
        ini_strip_comment(line);
        ini_trim_inplace(line);
        if (!line[0]) continue;
        if (line[0] == '[') {
            char *rbr;
            rbr = strchr(line, ']');
            if (rbr) {
                *rbr = '\0';
                ini_copy(section, line + 1, (int)sizeof(section));
                ini_trim_inplace(section);
            }
            continue;
        }
        eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        ini_copy(key, line, (int)sizeof(key));
        ini_copy(value, eq + 1, (int)sizeof(value));
        ini_trim_inplace(key);
        ini_trim_inplace(value);
        if (section[0] && gfa_ini_apply_pair(world, section, key, value)) ++applied;
    }
    return applied > 0 ? GFA_TRUE : GFA_FALSE;
}

int gfa_load_ini_file(GFA_World *world, const char *path)
{
    FILE *fp;
    char line[512];
    char section[128];
    char key[128];
    char value[256];
    char *eq;
    int applied;
    if (!world || !path) return GFA_FALSE;
    fp = fopen(path, "rb");
    if (!fp) return GFA_FALSE;
    section[0] = '\0';
    applied = 0;
    while (fgets(line, (int)sizeof(line), fp)) {
        ini_strip_comment(line);
        ini_trim_inplace(line);
        if (!line[0]) continue;
        if (line[0] == '[') {
            char *rbr;
            rbr = strchr(line, ']');
            if (rbr) {
                *rbr = '\0';
                ini_copy(section, line + 1, (int)sizeof(section));
                ini_trim_inplace(section);
            }
            continue;
        }
        eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        ini_copy(key, line, (int)sizeof(key));
        ini_copy(value, eq + 1, (int)sizeof(value));
        ini_trim_inplace(key);
        ini_trim_inplace(value);
        if (section[0] && gfa_ini_apply_pair(world, section, key, value)) ++applied;
    }
    fclose(fp);
    return applied > 0 ? GFA_TRUE : GFA_FALSE;
}
