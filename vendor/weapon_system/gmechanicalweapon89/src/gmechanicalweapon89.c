#include "gmechanicalweapon89.h"

#include <string.h>

#define GMW89_RIG_TAG 3919

static nm89_fx b3d_mech89_lerp_segment(nm89_fx from,
                                       nm89_fx to,
                                       unsigned long numerator,
                                       unsigned long denominator)
{
    nm89_fx t;
    if (denominator == 0UL || numerator >= denominator) return to;
    t = nm89_fx_from_ratio((nm89_i32)numerator,
                           (nm89_i32)denominator);
    return nm89_lerp(from, to, t);
}

static nm89_transform b3d_mech89_transform(nm89_fx x,
                                           nm89_fx y,
                                           nm89_fx z)
{
    nm89_transform value;
    nm89_transform_identity(&value);
    value.move.x = x;
    value.move.y = y;
    value.move.z = z;
    return value;
}

static nm89_fx b3d_mech89_q12_to_q16(gatt_fix value)
{
    long converted;
    converted = (long)value * 16L;
    if (converted > 2147483647L) return (nm89_fx)2147483647L;
    if (converted < (-2147483647L - 1L))
        return (nm89_fx)(-2147483647L - 1L);
    return (nm89_fx)converted;
}

static void b3d_mech89_attachment_to_matrix(
    const GAtt89_Xform *xform,
    nm89_matrix *out_matrix)
{
    GAtt89_Quat q;
    gatt_fix xx;
    gatt_fix yy;
    gatt_fix zz;
    gatt_fix xy;
    gatt_fix xz;
    gatt_fix yz;
    gatt_fix wx;
    gatt_fix wy;
    gatt_fix wz;
    gatt_fix two;
    gatt_fix m00;
    gatt_fix m01;
    gatt_fix m02;
    gatt_fix m10;
    gatt_fix m11;
    gatt_fix m12;
    gatt_fix m20;
    gatt_fix m21;
    gatt_fix m22;

    if (out_matrix == 0) return;
    nm89_matrix_identity(out_matrix);
    if (xform == 0) return;
    q = xform->rot;
    two = GATTACH89_FIX_ONE * 2;
    xx = gatt89_mul(q.x, q.x);
    yy = gatt89_mul(q.y, q.y);
    zz = gatt89_mul(q.z, q.z);
    xy = gatt89_mul(q.x, q.y);
    xz = gatt89_mul(q.x, q.z);
    yz = gatt89_mul(q.y, q.z);
    wx = gatt89_mul(q.w, q.x);
    wy = gatt89_mul(q.w, q.y);
    wz = gatt89_mul(q.w, q.z);

    m00 = GATTACH89_FIX_ONE - gatt89_mul(two, yy + zz);
    m01 = gatt89_mul(two, xy - wz);
    m02 = gatt89_mul(two, xz + wy);
    m10 = gatt89_mul(two, xy + wz);
    m11 = GATTACH89_FIX_ONE - gatt89_mul(two, xx + zz);
    m12 = gatt89_mul(two, yz - wx);
    m20 = gatt89_mul(two, xz - wy);
    m21 = gatt89_mul(two, yz + wx);
    m22 = GATTACH89_FIX_ONE - gatt89_mul(two, xx + yy);

    out_matrix->m[0][0] = b3d_mech89_q12_to_q16(
        gatt89_mul(m00, xform->scale.x));
    out_matrix->m[0][1] = b3d_mech89_q12_to_q16(
        gatt89_mul(m01, xform->scale.y));
    out_matrix->m[0][2] = b3d_mech89_q12_to_q16(
        gatt89_mul(m02, xform->scale.z));
    out_matrix->m[0][3] = b3d_mech89_q12_to_q16(xform->pos.x);
    out_matrix->m[1][0] = b3d_mech89_q12_to_q16(
        gatt89_mul(m10, xform->scale.x));
    out_matrix->m[1][1] = b3d_mech89_q12_to_q16(
        gatt89_mul(m11, xform->scale.y));
    out_matrix->m[1][2] = b3d_mech89_q12_to_q16(
        gatt89_mul(m12, xform->scale.z));
    out_matrix->m[1][3] = b3d_mech89_q12_to_q16(xform->pos.y);
    out_matrix->m[2][0] = b3d_mech89_q12_to_q16(
        gatt89_mul(m20, xform->scale.x));
    out_matrix->m[2][1] = b3d_mech89_q12_to_q16(
        gatt89_mul(m21, xform->scale.y));
    out_matrix->m[2][2] = b3d_mech89_q12_to_q16(
        gatt89_mul(m22, xform->scale.z));
    out_matrix->m[2][3] = b3d_mech89_q12_to_q16(xform->pos.z);
}

static int b3d_mech89_sample_world(void *user,
                                   const nm89_rig *rig,
                                   nm89_i16 part_id,
                                   nm89_i16 source_id,
                                   nm89_matrix *out_world)
{
    GMechanicalWeapon89 *weapon;
    (void)rig;
    (void)part_id;
    (void)source_id;
    weapon = (GMechanicalWeapon89 *)user;
    if (weapon == 0 || out_world == 0 || !weapon->attachment_valid)
        return NM89_ERR_PROVIDER;
    *out_world = weapon->attachment_world;
    return NM89_OK;
}

static int b3d_mech89_sample_transform(void *user,
                                       const nm89_rig *rig,
                                       nm89_i16 part_id,
                                       nm89_i16 source_id,
                                       nm89_transform_sample *out_sample)
{
    GMechanicalWeapon89 *weapon;
    unsigned long elapsed;
    unsigned long total;
    unsigned long p30;
    unsigned long p38;
    unsigned long p55;
    nm89_fx home_y;
    nm89_fx out_y;
    nm89_fx home_z;
    nm89_fx out_z;

    (void)rig;
    (void)part_id;
    weapon = (GMechanicalWeapon89 *)user;
    if (weapon == 0 || out_sample == 0 ||
        source_id != GMW89_PART_MAGAZINE)
        return NM89_ERR_PROVIDER;

    nm89_transform_sample_identity(out_sample);
    home_y = b3d_mech89_q12_to_q16(weapon->presentation.feed_home_y);
    home_z = b3d_mech89_q12_to_q16(weapon->presentation.feed_home_z);
    out_y = home_y;
    out_z = home_z;

    if (weapon->reload_active && weapon->reload_total_ms > 0U) {
        elapsed = (unsigned long)weapon->reload_elapsed_ms;
        total = (unsigned long)weapon->reload_total_ms;
        if (elapsed > total) elapsed = total;
        p30 = (total * 30UL) / 100UL;
        p38 = (total * 38UL) / 100UL;
        p55 = (total * 55UL) / 100UL;

        if (elapsed <= p30 && p30 > 0UL) {
            out_y = b3d_mech89_lerp_segment(
                home_y, b3d_mech89_q12_to_q16(
                    weapon->presentation.feed_out_y),
                elapsed, p30);
            out_z = b3d_mech89_lerp_segment(
                home_z, b3d_mech89_q12_to_q16(
                    weapon->presentation.feed_out_z),
                elapsed, p30);
        } else if (elapsed < p55) {
            out_y = b3d_mech89_q12_to_q16(
                weapon->presentation.feed_out_y);
            out_z = b3d_mech89_q12_to_q16(
                weapon->presentation.feed_out_z);
        } else if (total > p55) {
            out_y = b3d_mech89_lerp_segment(
                b3d_mech89_q12_to_q16(
                    weapon->presentation.feed_out_y), home_y,
                elapsed - p55, total - p55);
            out_z = b3d_mech89_lerp_segment(
                b3d_mech89_q12_to_q16(
                    weapon->presentation.feed_out_z), home_z,
                elapsed - p55, total - p55);
        }

        out_sample->has_visibility = 1U;
        out_sample->visible =
            weapon->presentation.detachable_magazine &&
            (elapsed >= p38 && elapsed < p55) ? 0U : 1U;
    } else {
        out_sample->has_visibility = 1U;
        out_sample->visible = 1U;
    }

    out_sample->transform.move.y = out_y;
    out_sample->transform.move.z = out_z;
    out_sample->channels = NM89_CHANNEL_MOVE_Y |
                           NM89_CHANNEL_MOVE_Z;
    return NM89_OK;
}

static void b3d_mech89_apply_geometry(void *user,
                                      const nm89_rig *rig,
                                      const nm89_geometry_packet *packet)
{
    GMechanicalWeapon89 *weapon;
    (void)rig;
    weapon = (GMechanicalWeapon89 *)user;
    if (weapon == 0 || packet == 0) return;
    if (weapon->packet_count >= GMW89_MAX_PACKETS) return;
    weapon->packets[weapon->packet_count] = *packet;
    weapon->packet_count += 1;
}

static void b3d_mech89_emit_event(void *user,
                                  const nm89_rig *rig,
                                  const nm89_clip_event *event_value)
{
    GMechanicalWeapon89 *weapon;
    (void)rig;
    (void)event_value;
    weapon = (GMechanicalWeapon89 *)user;
    if (weapon != 0) weapon->emitted_event_count += 1;
}

static int b3d_mech89_add_constraint(nm89_rig *rig,
                                     const nm89_transform *home,
                                     int property_a,
                                     nm89_fx min_a,
                                     nm89_fx max_a,
                                     int property_b,
                                     nm89_fx min_b,
                                     nm89_fx max_b,
                                     nm89_i16 *out_id)
{
    nm89_constraint constraint_value;
    int result;
    nm89_constraint_lock_to_transform(&constraint_value, home);
    if (property_a >= 0) {
        result = nm89_constraint_set(&constraint_value, property_a,
                                     NM89_CONSTRAINT_CLAMPED,
                                     min_a, max_a, NM89_FX_ONE);
        if (result != NM89_OK) return result;
    }
    if (property_b >= 0) {
        result = nm89_constraint_set(&constraint_value, property_b,
                                     NM89_CONSTRAINT_CLAMPED,
                                     min_b, max_b, NM89_FX_ONE);
        if (result != NM89_OK) return result;
    }
    return nm89_constraint_add(rig, &constraint_value, out_id);
}

static int b3d_mech89_add_binding(GMechanicalWeapon89 *weapon,
                                  nm89_i16 part_id,
                                  int mesh_id)
{
    int model_id;
    model_id = weapon->presentation.model_id;
    if (model_id < -32768) model_id = -32768;
    if (model_id > 32767) model_id = 32767;
    /* resource_id identifies the model selected by GWeapon/presentation;
       mesh_id identifies the animated part inside that model.  The renderer
       may resolve either a real multipart asset or the procedural fallback. */
    return nm89_binding_add(&weapon->rig,
                            part_id,
                            (nm89_i16)model_id,
                            NM89_SELECTOR_ENGINE_MESH,
                            (nm89_i16)mesh_id,
                            (nm89_i16)mesh_id,
                            NM89_INVALID_ID,
                            NM89_INVALID_ID,
                            (nm89_i16)mesh_id,
                            0);
}

static int b3d_mech89_build_fire_clip(GMechanicalWeapon89 *weapon)
{
    nm89_i16 track;
    nm89_clip_event event_value;
    nm89_u16 duration;
    nm89_u16 strike_tick;
    nm89_u16 return_tick;
    nm89_fx recoil_z;
    nm89_fx recoil_pitch;
    nm89_fx action_home;
    nm89_fx action_fire;
    int result;

    duration = weapon->presentation.fire_ticks;
    if (duration < 2U) duration = 2U;
    strike_tick = 1U;
    return_tick = duration > 1U ? (nm89_u16)(duration - 1U) : duration;
    recoil_z = b3d_mech89_q12_to_q16(weapon->presentation.recoil_z);
    recoil_pitch = b3d_mech89_q12_to_q16(
        weapon->presentation.recoil_pitch);
    action_home = b3d_mech89_q12_to_q16(
        weapon->presentation.action_home_z);
    action_fire = b3d_mech89_q12_to_q16(
        weapon->presentation.action_fire_z);

    result = nm89_clip_begin(&weapon->rig, GMW89_ACTION_FIRE,
                             duration, 0U, &weapon->fire_clip);
    if (result != NM89_OK) return result;

    result = nm89_clip_add_track(&weapon->rig, weapon->recoil_part,
                                 NM89_PROP_MOVE_Z,
                                 NM89_INTERP_SMOOTH, &track);
    if (result != NM89_OK) return result;
    if (nm89_track_add_key(&weapon->rig, track, 0U, 0) != NM89_OK ||
        nm89_track_add_key(&weapon->rig, track, strike_tick,
                           recoil_z) != NM89_OK ||
        nm89_track_add_key(&weapon->rig, track, duration, 0) != NM89_OK)
        return NM89_ERR_CAPACITY;

    result = nm89_clip_add_track(&weapon->rig, weapon->recoil_part,
                                 NM89_PROP_ROTATE_X,
                                 NM89_INTERP_SMOOTH, &track);
    if (result != NM89_OK) return result;
    if (nm89_track_add_key(&weapon->rig, track, 0U, 0) != NM89_OK ||
        nm89_track_add_key(&weapon->rig, track, strike_tick,
                           recoil_pitch) != NM89_OK ||
        nm89_track_add_key(&weapon->rig, track, duration, 0) != NM89_OK)
        return NM89_ERR_CAPACITY;

    if (weapon->presentation.mechanism_kind == GWPRES89_MECHANISM_REVOLVER) {
        result = nm89_clip_add_track(&weapon->rig, weapon->slide_part,
                                     NM89_PROP_ROTATE_Z,
                                     NM89_INTERP_SMOOTH, &track);
        if (result != NM89_OK) return result;
        if (nm89_track_add_key(&weapon->rig, track, 0U, 0) != NM89_OK ||
            nm89_track_add_key(&weapon->rig, track, strike_tick,
                               NM89_FX_FROM_INT(60)) != NM89_OK ||
            nm89_track_add_key(&weapon->rig, track, duration,
                               NM89_FX_FROM_INT(60)) != NM89_OK)
            return NM89_ERR_CAPACITY;
    } else if (weapon->presentation.mechanism_kind ==
               GWPRES89_MECHANISM_GATLING_BELT) {
        result = nm89_clip_add_track(&weapon->rig, weapon->barrel_part,
                                     NM89_PROP_ROTATE_Z,
                                     NM89_INTERP_LINEAR, &track);
        if (result != NM89_OK) return result;
        if (nm89_track_add_key(&weapon->rig, track, 0U, 0) != NM89_OK ||
            nm89_track_add_key(&weapon->rig, track, strike_tick,
                               NM89_FX_FROM_INT(120)) != NM89_OK ||
            nm89_track_add_key(&weapon->rig, track, duration,
                               NM89_FX_FROM_INT(360)) != NM89_OK)
            return NM89_ERR_CAPACITY;
    } else if (weapon->presentation.mechanism_kind != GWPRES89_MECHANISM_FIXED &&
               weapon->presentation.mechanism_kind != GWPRES89_MECHANISM_ELASTIC) {
        result = nm89_clip_add_track(&weapon->rig, weapon->slide_part,
                                     NM89_PROP_MOVE_Z,
                                     NM89_INTERP_SMOOTH, &track);
        if (result != NM89_OK) return result;
        if (nm89_track_add_key(&weapon->rig, track, 0U,
                               action_home) != NM89_OK ||
            nm89_track_add_key(&weapon->rig, track, strike_tick,
                               action_fire) != NM89_OK ||
            nm89_track_add_key(&weapon->rig, track, return_tick,
                               action_home) != NM89_OK)
            return NM89_ERR_CAPACITY;
    }

    memset(&event_value, 0, sizeof(event_value));
    event_value.tick = strike_tick;
    event_value.part_id = weapon->slide_part;
    event_value.code = GMW89_EVENT_SLIDE_REAR;
    event_value.value = weapon->presentation.mechanism_kind;
    event_value.type = NM89_EVENT_MARKER;
    result = nm89_clip_add_event(&weapon->rig, &event_value);
    if (result != NM89_OK) return result;

    result = nm89_clip_end(&weapon->rig);
    if (result != NM89_OK) return result;
    return nm89_action_bind(&weapon->rig,
                            GMW89_ACTION_FIRE,
                            weapon->fire_clip,
                            NM89_ACTION_RESTART);
}

int gmechanicalweapon89_init_profile(
    GMechanicalWeapon89 *weapon,
    const GWeaponPresentation89 *presentation)
{
    nm89_transform root_home;
    nm89_transform recoil_home;
    nm89_transform body_home;
    nm89_transform slide_home;
    nm89_transform magazine_home;
    nm89_transform barrel_home;
    nm89_transform muzzle_home;
    nm89_i16 root_constraint;
    nm89_i16 recoil_constraint;
    nm89_i16 body_constraint;
    nm89_i16 slide_constraint;
    nm89_i16 magazine_constraint;
    nm89_i16 barrel_constraint;
    nm89_i16 muzzle_constraint;
    int result;

    GWeaponPresentation89 fallback;

    if (weapon == 0) return 0;
    if (presentation == 0) {
        gweaponpresentation89_defaults(&fallback, 1, "weapon");
        presentation = &fallback;
    }
    memset(weapon, 0, sizeof(*weapon));
    weapon->presentation = *presentation;
    weapon->provider_id = NM89_INVALID_ID;
    weapon->root_part = NM89_INVALID_ID;
    weapon->recoil_part = NM89_INVALID_ID;
    weapon->body_part = NM89_INVALID_ID;
    weapon->slide_part = NM89_INVALID_ID;
    weapon->magazine_part = NM89_INVALID_ID;
    weapon->barrel_part = NM89_INVALID_ID;
    weapon->muzzle_socket_part = NM89_INVALID_ID;
    weapon->fire_clip = NM89_INVALID_ID;
    weapon->reload_total_ms = 1U;
    nm89_matrix_identity(&weapon->attachment_world);

    nm89_rig_init(&weapon->rig, GMW89_RIG_TAG);
    memset(&weapon->provider, 0, sizeof(weapon->provider));
    weapon->provider.user = weapon;
    weapon->provider.sample_transform = b3d_mech89_sample_transform;
    weapon->provider.sample_world = b3d_mech89_sample_world;
    weapon->provider.apply_geometry = b3d_mech89_apply_geometry;
    weapon->provider.emit_event = b3d_mech89_emit_event;
    result = nm89_provider_add(&weapon->rig, &weapon->provider,
                               &weapon->provider_id);
    if (result != NM89_OK) goto failed;

    root_home = b3d_mech89_transform(0, 0, 0);
    recoil_home = b3d_mech89_transform(0, 0, 0);
    body_home = b3d_mech89_transform(0, 0, 0);
    slide_home = b3d_mech89_transform(0, 0,
        b3d_mech89_q12_to_q16(weapon->presentation.action_home_z));
    magazine_home = b3d_mech89_transform(0,
        b3d_mech89_q12_to_q16(weapon->presentation.feed_home_y),
        b3d_mech89_q12_to_q16(weapon->presentation.feed_home_z));
    barrel_home = b3d_mech89_transform(0, 0,
        b3d_mech89_q12_to_q16(weapon->presentation.barrel_home_z));
    muzzle_home = b3d_mech89_transform(0,
        b3d_mech89_q12_to_q16(weapon->presentation.muzzle_y),
        b3d_mech89_q12_to_q16(weapon->presentation.muzzle_z));

    result = b3d_mech89_add_constraint(&weapon->rig, &root_home,
        -1, 0, 0, -1, 0, 0, &root_constraint);
    if (result != NM89_OK) goto failed;
    result = b3d_mech89_add_constraint(&weapon->rig, &recoil_home,
        NM89_PROP_MOVE_Z, 0,
        b3d_mech89_q12_to_q16(weapon->presentation.recoil_z),
        NM89_PROP_ROTATE_X,
        b3d_mech89_q12_to_q16(weapon->presentation.recoil_pitch) * 2, 0,
        &recoil_constraint);
    if (result != NM89_OK) goto failed;
    result = b3d_mech89_add_constraint(&weapon->rig, &body_home,
        -1, 0, 0, -1, 0, 0, &body_constraint);
    if (result != NM89_OK) goto failed;
    result = b3d_mech89_add_constraint(&weapon->rig, &slide_home,
        NM89_PROP_MOVE_Z,
        b3d_mech89_q12_to_q16(weapon->presentation.action_home_z),
        b3d_mech89_q12_to_q16(weapon->presentation.action_fire_z),
        -1, 0, 0, &slide_constraint);
    if (result != NM89_OK) goto failed;
    result = b3d_mech89_add_constraint(&weapon->rig, &magazine_home,
        NM89_PROP_MOVE_Y,
        b3d_mech89_q12_to_q16(weapon->presentation.feed_out_y),
        b3d_mech89_q12_to_q16(weapon->presentation.feed_home_y),
        NM89_PROP_MOVE_Z,
        b3d_mech89_q12_to_q16(weapon->presentation.feed_home_z),
        b3d_mech89_q12_to_q16(weapon->presentation.feed_out_z),
        &magazine_constraint);
    if (result != NM89_OK) goto failed;
    result = b3d_mech89_add_constraint(&weapon->rig, &barrel_home,
        -1, 0, 0, -1, 0, 0, &barrel_constraint);
    if (result != NM89_OK) goto failed;
    result = b3d_mech89_add_constraint(&weapon->rig, &muzzle_home,
        -1, 0, 0, -1, 0, 0, &muzzle_constraint);
    if (result != NM89_OK) goto failed;

    result = nm89_part_add(&weapon->rig, "weapon_root",
        NM89_ROOT_PART, GMW89_PART_ROOT, &root_home,
        root_constraint, NM89_INVALID_ID, 1U, &weapon->root_part);
    if (result != NM89_OK) goto failed;
    result = nm89_part_add(&weapon->rig, "recoil_carriage",
        weapon->root_part, GMW89_PART_RECOIL, &recoil_home,
        recoil_constraint, NM89_INVALID_ID, 1U, &weapon->recoil_part);
    if (result != NM89_OK) goto failed;
    result = nm89_part_add(&weapon->rig, "body",
        weapon->recoil_part, GMW89_PART_BODY, &body_home,
        body_constraint, NM89_INVALID_ID, 1U, &weapon->body_part);
    if (result != NM89_OK) goto failed;
    result = nm89_part_add(&weapon->rig, "slide",
        weapon->recoil_part, GMW89_PART_SLIDE, &slide_home,
        slide_constraint, NM89_INVALID_ID, 1U, &weapon->slide_part);
    if (result != NM89_OK) goto failed;
    result = nm89_part_add(&weapon->rig, "magazine",
        weapon->recoil_part, GMW89_PART_MAGAZINE, &magazine_home,
        magazine_constraint, NM89_INVALID_ID, 1U, &weapon->magazine_part);
    if (result != NM89_OK) goto failed;
    result = nm89_part_add(&weapon->rig, "barrel",
        weapon->recoil_part, GMW89_PART_BARREL, &barrel_home,
        barrel_constraint, NM89_INVALID_ID, 1U, &weapon->barrel_part);
    if (result != NM89_OK) goto failed;
    result = nm89_part_add(&weapon->rig, "muzzle_socket",
        weapon->recoil_part, GMW89_PART_MUZZLE_SOCKET, &muzzle_home,
        muzzle_constraint, NM89_INVALID_ID, 0U, &weapon->muzzle_socket_part);
    if (result != NM89_OK) goto failed;

    result = nm89_part_bind_world_provider(&weapon->rig,
        weapon->root_part, weapon->provider_id, 0,
        NM89_WORLD_REPLACE);
    if (result != NM89_OK) goto failed;
    result = nm89_part_bind_transform_provider(&weapon->rig,
        weapon->magazine_part, weapon->provider_id,
        GMW89_PART_MAGAZINE,
        NM89_CHANNEL_MOVE_Y | NM89_CHANNEL_MOVE_Z |
        NM89_CHANNEL_VISIBLE,
        NM89_PROVIDER_REPLACE);
    if (result != NM89_OK) goto failed;

    if (b3d_mech89_add_binding(weapon, weapon->body_part,
                               GMW89_MESH_BODY) != NM89_OK ||
        b3d_mech89_add_binding(weapon, weapon->slide_part,
                               GMW89_MESH_SLIDE) != NM89_OK ||
        b3d_mech89_add_binding(weapon, weapon->magazine_part,
                               GMW89_MESH_MAGAZINE) != NM89_OK ||
        b3d_mech89_add_binding(weapon, weapon->barrel_part,
                               GMW89_MESH_BARREL) != NM89_OK) {
        result = NM89_ERR_CAPACITY;
        goto failed;
    }

    result = b3d_mech89_build_fire_clip(weapon);
    if (result != NM89_OK) goto failed;

    weapon->attachment_valid = 1;
    weapon->current_weapon_id = weapon->presentation.weapon_id;
    weapon->packet_count = 0;
    result = nm89_step(&weapon->rig, 0U, 1U);
    if (result != NM89_OK) goto failed;
    weapon->initialized = 1;
    weapon->last_result = NM89_OK;
    return 1;

failed:
    weapon->last_result = result;
    weapon->initialized = 0;
    return 0;
}

int gmechanicalweapon89_init(GMechanicalWeapon89 *weapon)
{
    GWeaponPresentation89 presentation;
    gweaponpresentation89_defaults(&presentation, 1, "weapon");
    return gmechanicalweapon89_init_profile(weapon, &presentation);
}

void gmechanicalweapon89_set_attachment(
    GMechanicalWeapon89 *weapon,
    const GAtt89_Xform *attachment_world)
{
    if (weapon == 0 || attachment_world == 0) return;
    b3d_mech89_attachment_to_matrix(attachment_world,
                                    &weapon->attachment_world);
    weapon->attachment_valid = 1;
}

void gmechanicalweapon89_set_weapon(
    GMechanicalWeapon89 *weapon,
    int weapon_id)
{
    if (weapon == 0) return;
    if (weapon->current_weapon_id == weapon_id) return;
    weapon->current_weapon_id = weapon_id;
    nm89_stop(&weapon->rig);
    weapon->reload_active = 0;
    weapon->reload_elapsed_ms = 0U;
}

void gmechanicalweapon89_set_reload(
    GMechanicalWeapon89 *weapon,
    int active,
    unsigned short elapsed_ms,
    unsigned short total_ms)
{
    if (weapon == 0) return;
    weapon->reload_active = active ? 1 : 0;
    weapon->reload_elapsed_ms = elapsed_ms;
    weapon->reload_total_ms = total_ms > 0U ? total_ms : 1U;
}

void gmechanicalweapon89_trigger_fire(
    GMechanicalWeapon89 *weapon)
{
    if (weapon == 0 || !weapon->initialized) return;
    (void)nm89_trigger_action(&weapon->rig,
                              GMW89_ACTION_FIRE);
}

int gmechanicalweapon89_update(
    GMechanicalWeapon89 *weapon,
    unsigned short frame_ms)
{
    unsigned int total_ms;
    nm89_u16 delta_ticks;
    int result;
    if (weapon == 0 || !weapon->initialized) return 0;
    total_ms = weapon->tick_remainder_ms + (unsigned int)frame_ms;
    delta_ticks = (nm89_u16)(total_ms / GMW89_TICK_MS);
    weapon->tick_remainder_ms = total_ms % GMW89_TICK_MS;
    weapon->packet_count = 0;
    result = nm89_step(&weapon->rig, delta_ticks, 1U);
    weapon->last_result = result;
    return result == NM89_OK;
}

int gmechanicalweapon89_packet_count(
    const GMechanicalWeapon89 *weapon)
{
    return weapon != 0 ? weapon->packet_count : 0;
}

const nm89_geometry_packet *gmechanicalweapon89_packet(
    const GMechanicalWeapon89 *weapon,
    int index)
{
    if (weapon == 0 || index < 0 || index >= weapon->packet_count)
        return 0;
    return &weapon->packets[index];
}

const nm89_pose *gmechanicalweapon89_part_pose(
    const GMechanicalWeapon89 *weapon,
    int part_tag)
{
    int i;
    if (weapon == 0) return 0;
    for (i = 0; i < (int)weapon->rig.part_count; ++i) {
        if (weapon->rig.parts[i].user_tag == part_tag)
            return nm89_part_get_pose(&weapon->rig, (nm89_i16)i);
    }
    return 0;
}

const nm89_pose *gmechanicalweapon89_muzzle_socket(
    const GMechanicalWeapon89 *weapon)
{
    return gmechanicalweapon89_part_pose(
        weapon, GMW89_PART_MUZZLE_SOCKET);
}

const nm89_matrix *gmechanicalweapon89_muzzle_world(
    const GMechanicalWeapon89 *weapon)
{
    int i;
    if (weapon == 0) return 0;
    for (i = 0; i < (int)weapon->rig.part_count; ++i) {
        if (weapon->rig.parts[i].user_tag ==
            GMW89_PART_MUZZLE_SOCKET)
            return &weapon->rig.parts[i].world;
    }
    return 0;
}

const char *gmechanicalweapon89_status(
    const GMechanicalWeapon89 *weapon)
{
    if (weapon == 0) return "NationalMecanicanimal89 bridge: null";
    if (!weapon->initialized)
        return "NationalMecanicanimal89 bridge: initialization failed";
    if (weapon->last_result != NM89_OK)
        return "NationalMecanicanimal89 bridge: runtime error";
    if (weapon->reload_active)
        return "NationalMecanicanimal89 bridge: reload provider active";
    if (weapon->rig.player.playing)
        return "NationalMecanicanimal89 bridge: fire clip active";
    return "NationalMecanicanimal89 bridge: animating attached object";
}
