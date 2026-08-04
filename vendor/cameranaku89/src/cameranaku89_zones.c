/* cameranaku89_zones.c - CC0 1.0 Universal. */
#include "cameranaku89_zones.h"

static cnk_fx cnk_zone_min(cnk_fx a, cnk_fx b)
{
    return a < b ? a : b;
}

static cnk_fx cnk_zone_max(cnk_fx a, cnk_fx b)
{
    return a > b ? a : b;
}

CNK_API void cnk_camera_zone_bank_clear(cnk_camera_zone_bank *bank)
{
    int i;
    if (bank == 0) {
        return;
    }
    bank->count = 0;
    bank->active_zone_index = -1;
    for (i = 0; i < CNK_MAX_ZONES; ++i) {
        bank->zones[i].id = -1;
        bank->zones[i].vcam_id = -1;
        bank->zones[i].flags = 0;
        bank->zones[i].priority = 0;
        bank->zones[i].priority_bias = 0;
        bank->zones[i].force_ticks = 0;
        bank->zones[i].blend_ticks = 0;
        bank->zones[i].minv = cnk_vec3_make(0, 0, 0);
        bank->zones[i].maxv = cnk_vec3_make(0, 0, 0);
        bank->zones[i].confiner.minv = cnk_vec3_make(0, 0, 0);
        bank->zones[i].confiner.maxv = cnk_vec3_make(0, 0, 0);
        bank->zones[i].confiner.enabled = 0;
        bank->zones[i].inside = 0;
        bank->zones[i].was_inside = 0;
    }
}

CNK_API int cnk_camera_zone_add_box(cnk_camera_zone_bank *bank, int id, int vcam_id, cnk_vec3 minv, cnk_vec3 maxv, int priority, int flags)
{
    int i;
    if (bank == 0 || bank->count >= CNK_MAX_ZONES) {
        return -1;
    }
    i = bank->count;
    bank->zones[i].id = id;
    bank->zones[i].vcam_id = vcam_id;
    bank->zones[i].flags = flags;
    bank->zones[i].priority = priority;
    bank->zones[i].priority_bias = 0;
    bank->zones[i].force_ticks = 0;
    bank->zones[i].blend_ticks = 0;
    bank->zones[i].minv = minv;
    bank->zones[i].maxv = maxv;
    bank->zones[i].confiner.minv = minv;
    bank->zones[i].confiner.maxv = maxv;
    bank->zones[i].confiner.enabled = 0;
    bank->zones[i].inside = 0;
    bank->zones[i].was_inside = 0;
    bank->count += 1;
    return i;
}

CNK_API void cnk_camera_zone_set_bias(cnk_camera_zone_bank *bank, int index, int priority_bias, int force_ticks, int blend_ticks)
{
    if (bank == 0 || index < 0 || index >= bank->count) {
        return;
    }
    bank->zones[index].priority_bias = priority_bias;
    bank->zones[index].force_ticks = force_ticks;
    bank->zones[index].blend_ticks = blend_ticks;
}

CNK_API void cnk_camera_zone_set_confiner(cnk_camera_zone_bank *bank, int index, cnk_vec3 minv, cnk_vec3 maxv)
{
    if (bank == 0 || index < 0 || index >= bank->count) {
        return;
    }
    bank->zones[index].confiner.minv = minv;
    bank->zones[index].confiner.maxv = maxv;
    bank->zones[index].confiner.enabled = 1;
    bank->zones[index].flags |= CNK_ZONE_APPLY_CONFINER;
}

CNK_API int cnk_camera_zone_contains(const cnk_camera_zone *zone, cnk_vec3 point)
{
    cnk_fx minx;
    cnk_fx miny;
    cnk_fx minz;
    cnk_fx maxx;
    cnk_fx maxy;
    cnk_fx maxz;
    if (zone == 0) {
        return CNK_FALSE;
    }
    minx = cnk_zone_min(zone->minv.x, zone->maxv.x);
    miny = cnk_zone_min(zone->minv.y, zone->maxv.y);
    minz = cnk_zone_min(zone->minv.z, zone->maxv.z);
    maxx = cnk_zone_max(zone->minv.x, zone->maxv.x);
    maxy = cnk_zone_max(zone->minv.y, zone->maxv.y);
    maxz = cnk_zone_max(zone->minv.z, zone->maxv.z);
    if (point.x < minx || point.x > maxx) {
        return CNK_FALSE;
    }
    if (point.y < miny || point.y > maxy) {
        return CNK_FALSE;
    }
    if (point.z < minz || point.z > maxz) {
        return CNK_FALSE;
    }
    return CNK_TRUE;
}

CNK_API cnk_camera_zone_report cnk_camera_zone_bank_update(cnk_camera_zone_bank *bank, cnk_vec3 point)
{
    cnk_camera_zone_report report;
    int i;
    int best;
    int best_priority;
    report.zone_index = -1;
    report.zone_id = -1;
    report.vcam_id = -1;
    report.entered = 0;
    report.exited = 0;
    report.inside = 0;
    best = -1;
    best_priority = -32768;
    if (bank == 0) {
        return report;
    }
    for (i = 0; i < bank->count; ++i) {
        bank->zones[i].was_inside = bank->zones[i].inside;
        bank->zones[i].inside = cnk_camera_zone_contains(&bank->zones[i], point);
        if (bank->zones[i].was_inside == 0 && bank->zones[i].inside != 0) {
            report.entered = 1;
        }
        if (bank->zones[i].was_inside != 0 && bank->zones[i].inside == 0) {
            report.exited = 1;
        }
        if (bank->zones[i].inside != 0) {
            if (best < 0 || bank->zones[i].priority > best_priority) {
                best = i;
                best_priority = bank->zones[i].priority;
            }
        }
    }
    bank->active_zone_index = best;
    if (best >= 0) {
        report.zone_index = best;
        report.zone_id = bank->zones[best].id;
        report.vcam_id = bank->zones[best].vcam_id;
        report.inside = 1;
    }
    return report;
}

CNK_API int cnk_camera_zone_bank_apply(cnk_camera_zone_bank *bank, cnk_camera_manager *mgr, cnk_vec3 point)
{
    cnk_camera_zone_report report;
    cnk_camera_zone *zone;
    cnk_virtual_camera *vcam;
    int old_blend;
    if (bank == 0 || mgr == 0) {
        return CNK_FALSE;
    }
    cnk_camera_manager_clear_priority_biases(mgr);
    report = cnk_camera_zone_bank_update(bank, point);
    if (report.zone_index < 0) {
        return CNK_FALSE;
    }
    zone = &bank->zones[report.zone_index];
    vcam = cnk_camera_manager_find_by_id(mgr, zone->vcam_id);
    if (vcam == 0) {
        return CNK_FALSE;
    }
    vcam->priority_bias = zone->priority_bias;
    if ((zone->flags & CNK_ZONE_ENABLE_CAMERA) != 0) {
        vcam->state = CNK_VCAM_STANDBY;
    }
    if ((zone->flags & CNK_ZONE_APPLY_CONFINER) != 0 && zone->confiner.enabled != 0) {
        cnk_collision_set_confiner_box(&vcam->collision, zone->confiner.minv, zone->confiner.maxv);
        vcam->use_collision_ext = 1;
    }
    if ((zone->flags & CNK_ZONE_FORCE_CAMERA) != 0) {
        old_blend = mgr->default_blend_ticks;
        if ((zone->flags & CNK_ZONE_USE_BLEND) != 0 && zone->blend_ticks >= 0) {
            mgr->default_blend_ticks = zone->blend_ticks;
        }
        cnk_camera_manager_force_by_id(mgr, zone->vcam_id, zone->force_ticks);
        if ((zone->flags & CNK_ZONE_USE_BLEND) == 0) {
            mgr->default_blend_ticks = old_blend;
        }
    }
    return CNK_TRUE;
}
