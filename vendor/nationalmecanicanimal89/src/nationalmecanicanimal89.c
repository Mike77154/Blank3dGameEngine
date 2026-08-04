#include "nationalmecanicanimal89.h"
#include "nm89_sin_table.h"

static void nm89_copy_name(char *destination, const char *source)
{
    int i;
    if (destination == 0) {
        return;
    }
    i = 0;
    if (source != 0) {
        while (source[i] != '\0' && i < NM89_NAME_CAPACITY - 1) {
            destination[i] = source[i];
            i += 1;
        }
    }
    destination[i] = '\0';
}

static int nm89_names_equal(const char *a, const char *b)
{
    int i;
    if (a == 0 || b == 0) {
        return 0;
    }
    i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        i += 1;
    }
    return a[i] == b[i];
}

static int nm89_valid_part(const nm89_rig *rig, nm89_i16 part_id)
{
    return rig != 0 && part_id >= 0 &&
           (nm89_u16)part_id < rig->part_count;
}

static int nm89_valid_provider(const nm89_rig *rig, nm89_i16 provider_id)
{
    return rig != 0 && provider_id >= 0 &&
           (nm89_u16)provider_id < rig->provider_count;
}

static int nm89_valid_constraint(const nm89_rig *rig,
                                 nm89_i16 constraint_id)
{
    return rig != 0 && constraint_id >= 0 &&
           (nm89_u16)constraint_id < rig->constraint_count;
}

static int nm89_valid_pivot(const nm89_rig *rig, nm89_i16 pivot_id)
{
    return rig != 0 && pivot_id >= 0 &&
           (nm89_u16)pivot_id < rig->pivot_count;
}

static int nm89_valid_alignment(const nm89_rig *rig,
                                nm89_i16 alignment_id)
{
    return rig != 0 && alignment_id >= 0 &&
           (nm89_u16)alignment_id < rig->alignment_count;
}

static void nm89_log_all(const nm89_rig *rig, int level,
                         int code, const char *message)
{
    nm89_u16 i;
    if (rig == 0) {
        return;
    }
    for (i = 0; i < rig->provider_count; ++i) {
        if (rig->providers[i].log != 0) {
            rig->providers[i].log(rig->providers[i].user,
                                  level, code, message);
        }
    }
}

nm89_fx nm89_fx_from_int(nm89_i32 value)
{
    return (nm89_fx)(value * NM89_FX_ONE);
}

nm89_i32 nm89_fx_to_int(nm89_fx value)
{
    return (nm89_i32)(value / NM89_FX_ONE);
}

nm89_fx nm89_fx_from_ratio(nm89_i32 numerator, nm89_i32 denominator)
{
    nm89_i32 quotient;
    nm89_i32 remainder;
    if (denominator == 0) {
        return 0;
    }
    quotient = numerator / denominator;
    remainder = numerator % denominator;
    return (nm89_fx)(quotient * NM89_FX_ONE +
                     (remainder * NM89_FX_ONE) / denominator);
}

nm89_fx nm89_mul(nm89_fx a, nm89_fx b)
{
    nm89_i32 a_high;
    nm89_i32 a_low;
    nm89_i32 b_high;
    nm89_i32 b_low;
    nm89_i32 low_term;
    nm89_i32 result;
    nm89_u32 abs_a_low;
    nm89_u32 abs_b_low;
    nm89_u32 low_product;

    a_high = a / NM89_FX_ONE;
    a_low = a % NM89_FX_ONE;
    b_high = b / NM89_FX_ONE;
    b_low = b % NM89_FX_ONE;

    abs_a_low = a_low < 0 ?
        (nm89_u32)(-a_low) : (nm89_u32)a_low;
    abs_b_low = b_low < 0 ?
        (nm89_u32)(-b_low) : (nm89_u32)b_low;
    low_product = (abs_a_low * abs_b_low) /
                  (nm89_u32)NM89_FX_ONE;
    low_term = (nm89_i32)low_product;
    if ((a_low < 0) != (b_low < 0)) {
        low_term = -low_term;
    }

    result = a_high * b_high * NM89_FX_ONE;
    result += a_high * b_low;
    result += b_high * a_low;
    result += low_term;
    return result;
}

nm89_fx nm89_div(nm89_fx a, nm89_fx b)
{
    nm89_i32 quotient;
    nm89_i32 remainder;
    if (b == 0) {
        return 0;
    }
    quotient = a / b;
    remainder = a % b;
    return quotient * NM89_FX_ONE +
           (remainder * NM89_FX_ONE) / b;
}

nm89_fx nm89_lerp(nm89_fx a, nm89_fx b, nm89_fx t)
{
    return a + nm89_mul(b - a, t);
}

nm89_fx nm89_sin_deg(nm89_fx degrees)
{
    nm89_i32 whole;
    nm89_i32 fraction;
    int index0;
    int index1;
    nm89_fx value0;
    nm89_fx value1;

    whole = degrees / NM89_FX_ONE;
    fraction = degrees % NM89_FX_ONE;
    if (fraction < 0) {
        fraction += NM89_FX_ONE;
        whole -= 1;
    }
    whole %= 360;
    if (whole < 0) {
        whole += 360;
    }
    index0 = (int)whole;
    index1 = (index0 + 1) % 360;
    value0 = g_nm89_sin_q16_16[index0];
    value1 = g_nm89_sin_q16_16[index1];
    return value0 + ((value1 - value0) * fraction) / NM89_FX_ONE;
}

nm89_fx nm89_cos_deg(nm89_fx degrees)
{
    return nm89_sin_deg(degrees + NM89_FX_FROM_INT(90));
}

void nm89_vec3_zero(nm89_vec3 *value)
{
    if (value == 0) {
        return;
    }
    value->x = 0;
    value->y = 0;
    value->z = 0;
}

void nm89_transform_identity(nm89_transform *value)
{
    if (value == 0) {
        return;
    }
    nm89_vec3_zero(&value->move);
    nm89_vec3_zero(&value->rotate);
    value->scale.x = NM89_FX_ONE;
    value->scale.y = NM89_FX_ONE;
    value->scale.z = NM89_FX_ONE;
}

void nm89_transform_sample_identity(nm89_transform_sample *sample)
{
    if (sample == 0) {
        return;
    }
    nm89_transform_identity(&sample->transform);
    sample->channels = 0UL;
    sample->visible = 1U;
    sample->has_visibility = 0U;
}

void nm89_matrix_identity(nm89_matrix *matrix)
{
    int row;
    int column;
    if (matrix == 0) {
        return;
    }
    for (row = 0; row < 4; ++row) {
        for (column = 0; column < 4; ++column) {
            matrix->m[row][column] =
                row == column ? NM89_FX_ONE : 0;
        }
    }
}

void nm89_matrix_multiply(nm89_matrix *out_matrix,
                          const nm89_matrix *a,
                          const nm89_matrix *b)
{
    nm89_matrix temporary;
    int row;
    int column;
    int k;
    nm89_fx sum;

    if (out_matrix == 0 || a == 0 || b == 0) {
        return;
    }
    for (row = 0; row < 4; ++row) {
        for (column = 0; column < 4; ++column) {
            sum = 0;
            for (k = 0; k < 4; ++k) {
                sum += nm89_mul(a->m[row][k], b->m[k][column]);
            }
            temporary.m[row][column] = sum;
        }
    }
    *out_matrix = temporary;
}

void nm89_matrix_from_transform(nm89_matrix *out_matrix,
                                const nm89_transform *transform_value,
                                const nm89_vec3 *pivot)
{
    nm89_matrix scale;
    nm89_matrix rotate_x;
    nm89_matrix rotate_y;
    nm89_matrix rotate_z;
    nm89_matrix translate_negative_pivot;
    nm89_matrix translate_position_pivot;
    nm89_matrix temporary_a;
    nm89_matrix temporary_b;
    nm89_vec3 zero_pivot;
    const nm89_vec3 *active_pivot;
    nm89_fx sx;
    nm89_fx cx;
    nm89_fx sy;
    nm89_fx cy;
    nm89_fx sz;
    nm89_fx cz;

    if (out_matrix == 0 || transform_value == 0) {
        return;
    }
    nm89_vec3_zero(&zero_pivot);
    active_pivot = pivot != 0 ? pivot : &zero_pivot;

    sx = nm89_sin_deg(transform_value->rotate.x);
    cx = nm89_cos_deg(transform_value->rotate.x);
    sy = nm89_sin_deg(transform_value->rotate.y);
    cy = nm89_cos_deg(transform_value->rotate.y);
    sz = nm89_sin_deg(transform_value->rotate.z);
    cz = nm89_cos_deg(transform_value->rotate.z);

    nm89_matrix_identity(&scale);
    scale.m[0][0] = transform_value->scale.x;
    scale.m[1][1] = transform_value->scale.y;
    scale.m[2][2] = transform_value->scale.z;

    nm89_matrix_identity(&rotate_x);
    rotate_x.m[1][1] = cx;
    rotate_x.m[1][2] = -sx;
    rotate_x.m[2][1] = sx;
    rotate_x.m[2][2] = cx;

    nm89_matrix_identity(&rotate_y);
    rotate_y.m[0][0] = cy;
    rotate_y.m[0][2] = sy;
    rotate_y.m[2][0] = -sy;
    rotate_y.m[2][2] = cy;

    nm89_matrix_identity(&rotate_z);
    rotate_z.m[0][0] = cz;
    rotate_z.m[0][1] = -sz;
    rotate_z.m[1][0] = sz;
    rotate_z.m[1][1] = cz;

    nm89_matrix_identity(&translate_negative_pivot);
    translate_negative_pivot.m[0][3] = -active_pivot->x;
    translate_negative_pivot.m[1][3] = -active_pivot->y;
    translate_negative_pivot.m[2][3] = -active_pivot->z;

    nm89_matrix_identity(&translate_position_pivot);
    translate_position_pivot.m[0][3] =
        transform_value->move.x + active_pivot->x;
    translate_position_pivot.m[1][3] =
        transform_value->move.y + active_pivot->y;
    translate_position_pivot.m[2][3] =
        transform_value->move.z + active_pivot->z;

    nm89_matrix_multiply(&temporary_a, &scale,
                         &translate_negative_pivot);
    nm89_matrix_multiply(&temporary_b, &rotate_x, &temporary_a);
    nm89_matrix_multiply(&temporary_a, &rotate_y, &temporary_b);
    nm89_matrix_multiply(&temporary_b, &rotate_z, &temporary_a);
    nm89_matrix_multiply(out_matrix, &translate_position_pivot,
                         &temporary_b);
}

void nm89_matrix_transform_point(const nm89_matrix *matrix,
                                 nm89_fx x, nm89_fx y, nm89_fx z,
                                 nm89_fx *out_x,
                                 nm89_fx *out_y,
                                 nm89_fx *out_z)
{
    if (matrix == 0 || out_x == 0 || out_y == 0 || out_z == 0) {
        return;
    }
    *out_x = nm89_mul(matrix->m[0][0], x) +
             nm89_mul(matrix->m[0][1], y) +
             nm89_mul(matrix->m[0][2], z) +
             matrix->m[0][3];
    *out_y = nm89_mul(matrix->m[1][0], x) +
             nm89_mul(matrix->m[1][1], y) +
             nm89_mul(matrix->m[1][2], z) +
             matrix->m[1][3];
    *out_z = nm89_mul(matrix->m[2][0], x) +
             nm89_mul(matrix->m[2][1], y) +
             nm89_mul(matrix->m[2][2], z) +
             matrix->m[2][3];
}

static nm89_provider *nm89_get_provider(nm89_rig *rig,
                                        nm89_i16 provider_id)
{
    if (!nm89_valid_provider(rig, provider_id)) {
        return 0;
    }
    return &rig->providers[provider_id];
}

static const nm89_provider *nm89_get_provider_const(
    const nm89_rig *rig, nm89_i16 provider_id)
{
    if (!nm89_valid_provider(rig, provider_id)) {
        return 0;
    }
    return &rig->providers[provider_id];
}

static void nm89_math_identity(const nm89_rig *rig,
                               nm89_matrix *out_matrix)
{
    const nm89_provider *provider;
    provider = nm89_get_provider_const(rig, rig->math_provider);
    if (provider != 0 && provider->matrix_identity != 0) {
        provider->matrix_identity(provider->user, out_matrix);
        return;
    }
    nm89_matrix_identity(out_matrix);
}

static void nm89_math_multiply(const nm89_rig *rig,
                               nm89_matrix *out_matrix,
                               const nm89_matrix *a,
                               const nm89_matrix *b)
{
    const nm89_provider *provider;
    provider = nm89_get_provider_const(rig, rig->math_provider);
    if (provider != 0 && provider->matrix_multiply != 0) {
        provider->matrix_multiply(provider->user, out_matrix, a, b);
        return;
    }
    nm89_matrix_multiply(out_matrix, a, b);
}

static void nm89_math_from_transform(const nm89_rig *rig,
                                     nm89_matrix *out_matrix,
                                     const nm89_transform *transform_value,
                                     const nm89_vec3 *pivot)
{
    const nm89_provider *provider;
    provider = nm89_get_provider_const(rig, rig->math_provider);
    if (provider != 0 && provider->matrix_from_transform != 0) {
        provider->matrix_from_transform(provider->user, out_matrix,
                                        transform_value, pivot);
        return;
    }
    nm89_matrix_from_transform(out_matrix, transform_value, pivot);
}

void nm89_rig_transform_point(const nm89_rig *rig,
                              const nm89_matrix *matrix,
                              nm89_fx x, nm89_fx y, nm89_fx z,
                              nm89_fx *out_x,
                              nm89_fx *out_y,
                              nm89_fx *out_z)
{
    const nm89_provider *provider;
    if (rig == 0) {
        nm89_matrix_transform_point(matrix, x, y, z,
                                    out_x, out_y, out_z);
        return;
    }
    provider = nm89_get_provider_const(rig, rig->math_provider);
    if (provider != 0 && provider->matrix_transform_point != 0) {
        provider->matrix_transform_point(provider->user, matrix,
                                         x, y, z,
                                         out_x, out_y, out_z);
        return;
    }
    nm89_matrix_transform_point(matrix, x, y, z,
                                out_x, out_y, out_z);
}

static nm89_axis_constraint *nm89_constraint_axis(
    nm89_constraint *constraint_value, int property)
{
    int axis;
    if (constraint_value == 0) {
        return 0;
    }
    if (property >= NM89_PROP_MOVE_X &&
        property <= NM89_PROP_MOVE_Z) {
        axis = property - NM89_PROP_MOVE_X;
        return &constraint_value->move[axis];
    }
    if (property >= NM89_PROP_ROTATE_X &&
        property <= NM89_PROP_ROTATE_Z) {
        axis = property - NM89_PROP_ROTATE_X;
        return &constraint_value->rotate[axis];
    }
    if (property >= NM89_PROP_SCALE_X &&
        property <= NM89_PROP_SCALE_Z) {
        axis = property - NM89_PROP_SCALE_X;
        return &constraint_value->scale[axis];
    }
    return 0;
}

static const nm89_axis_constraint *nm89_constraint_axis_const(
    const nm89_constraint *constraint_value, int property)
{
    int axis;
    if (constraint_value == 0) {
        return 0;
    }
    if (property >= NM89_PROP_MOVE_X &&
        property <= NM89_PROP_MOVE_Z) {
        axis = property - NM89_PROP_MOVE_X;
        return &constraint_value->move[axis];
    }
    if (property >= NM89_PROP_ROTATE_X &&
        property <= NM89_PROP_ROTATE_Z) {
        axis = property - NM89_PROP_ROTATE_X;
        return &constraint_value->rotate[axis];
    }
    if (property >= NM89_PROP_SCALE_X &&
        property <= NM89_PROP_SCALE_Z) {
        axis = property - NM89_PROP_SCALE_X;
        return &constraint_value->scale[axis];
    }
    return 0;
}

static nm89_fx *nm89_transform_channel(nm89_transform *transform_value,
                                       int property)
{
    if (transform_value == 0) {
        return 0;
    }
    switch (property) {
    case NM89_PROP_MOVE_X: return &transform_value->move.x;
    case NM89_PROP_MOVE_Y: return &transform_value->move.y;
    case NM89_PROP_MOVE_Z: return &transform_value->move.z;
    case NM89_PROP_ROTATE_X: return &transform_value->rotate.x;
    case NM89_PROP_ROTATE_Y: return &transform_value->rotate.y;
    case NM89_PROP_ROTATE_Z: return &transform_value->rotate.z;
    case NM89_PROP_SCALE_X: return &transform_value->scale.x;
    case NM89_PROP_SCALE_Y: return &transform_value->scale.y;
    case NM89_PROP_SCALE_Z: return &transform_value->scale.z;
    default: return 0;
    }
}

static const nm89_fx *nm89_transform_channel_const(
    const nm89_transform *transform_value, int property)
{
    if (transform_value == 0) {
        return 0;
    }
    switch (property) {
    case NM89_PROP_MOVE_X: return &transform_value->move.x;
    case NM89_PROP_MOVE_Y: return &transform_value->move.y;
    case NM89_PROP_MOVE_Z: return &transform_value->move.z;
    case NM89_PROP_ROTATE_X: return &transform_value->rotate.x;
    case NM89_PROP_ROTATE_Y: return &transform_value->rotate.y;
    case NM89_PROP_ROTATE_Z: return &transform_value->rotate.z;
    case NM89_PROP_SCALE_X: return &transform_value->scale.x;
    case NM89_PROP_SCALE_Y: return &transform_value->scale.y;
    case NM89_PROP_SCALE_Z: return &transform_value->scale.z;
    default: return 0;
    }
}

static void nm89_axis_free(nm89_axis_constraint *axis)
{
    if (axis == 0) {
        return;
    }
    axis->minimum = 0;
    axis->maximum = 0;
    axis->step = NM89_FX_ONE;
    axis->mode = NM89_CONSTRAINT_FREE;
}

void nm89_constraint_free(nm89_constraint *constraint_value)
{
    int i;
    if (constraint_value == 0) {
        return;
    }
    for (i = 0; i < 3; ++i) {
        nm89_axis_free(&constraint_value->move[i]);
        nm89_axis_free(&constraint_value->rotate[i]);
        nm89_axis_free(&constraint_value->scale[i]);
    }
}

void nm89_constraint_lock_to_transform(nm89_constraint *constraint_value,
                                       const nm89_transform *transform_value)
{
    int property;
    nm89_axis_constraint *axis;
    const nm89_fx *channel;
    if (constraint_value == 0 || transform_value == 0) {
        return;
    }
    for (property = NM89_PROP_MOVE_X;
         property <= NM89_PROP_SCALE_Z;
         ++property) {
        axis = nm89_constraint_axis(constraint_value, property);
        channel = nm89_transform_channel_const(transform_value, property);
        axis->minimum = *channel;
        axis->maximum = *channel;
        axis->step = NM89_FX_ONE;
        axis->mode = NM89_CONSTRAINT_LOCKED;
    }
}

int nm89_constraint_set(nm89_constraint *constraint_value,
                        int property, int mode,
                        nm89_fx minimum, nm89_fx maximum,
                        nm89_fx step)
{
    nm89_axis_constraint *axis;
    if (constraint_value == 0 ||
        property < NM89_PROP_MOVE_X ||
        property > NM89_PROP_SCALE_Z ||
        mode < NM89_CONSTRAINT_LOCKED ||
        mode > NM89_CONSTRAINT_STEPPED) {
        return NM89_ERR_ARGUMENT;
    }
    if (mode != NM89_CONSTRAINT_FREE && maximum < minimum) {
        return NM89_ERR_RANGE;
    }
    if (mode == NM89_CONSTRAINT_STEPPED && step <= 0) {
        return NM89_ERR_RANGE;
    }
    axis = nm89_constraint_axis(constraint_value, property);
    axis->minimum = minimum;
    axis->maximum = maximum;
    axis->step = step;
    axis->mode = (nm89_u8)mode;
    return NM89_OK;
}

void nm89_pivot_identity(nm89_pivot *pivot)
{
    if (pivot == 0) {
        return;
    }
    nm89_vec3_zero(&pivot->point);
}

void nm89_alignment_identity(nm89_alignment *alignment)
{
    if (alignment == 0) {
        return;
    }
    nm89_transform_identity(&alignment->transform);
}

static void nm89_part_provider_defaults(nm89_part *part)
{
    if (part == 0) {
        return;
    }
    part->transform_provider = NM89_INVALID_ID;
    part->transform_source = NM89_INVALID_ID;
    part->transform_channels = NM89_CHANNEL_TRANSFORM;
    part->transform_mode = NM89_PROVIDER_REPLACE;
    part->socket_provider = NM89_INVALID_ID;
    part->socket_source = NM89_INVALID_ID;
    part->socket_space = NM89_SOCKET_PARENT_SPACE;
    part->pivot_provider = NM89_INVALID_ID;
    part->pivot_source = NM89_INVALID_ID;
    part->world_provider = NM89_INVALID_ID;
    part->world_source = NM89_INVALID_ID;
    part->world_mode = NM89_WORLD_PREMULTIPLY;
    part->visibility_provider = NM89_INVALID_ID;
    part->visibility_source = NM89_INVALID_ID;
    part->visibility_mode = NM89_VISIBILITY_AND;
    part->constraint_provider = NM89_INVALID_ID;
    part->constraint_source = NM89_INVALID_ID;
    part->dynamic_constraint_mode =
        NM89_DYNAMIC_CONSTRAINT_INTERSECT;
}

void nm89_rig_init(nm89_rig *rig, nm89_i16 rig_tag)
{
    int i;
    if (rig == 0) {
        return;
    }
    rig->rig_tag = rig_tag;
    rig->part_count = 0U;
    rig->constraint_count = 0U;
    rig->pivot_count = 0U;
    rig->alignment_count = 0U;
    rig->binding_count = 0U;
    rig->clip_count = 0U;
    rig->track_count = 0U;
    rig->key_count = 0U;
    rig->event_count = 0U;
    rig->action_count = 0U;
    rig->provider_count = 0U;
    rig->builder_clip = NM89_INVALID_ID;
    rig->builder_track = NM89_INVALID_ID;
    rig->math_provider = NM89_INVALID_ID;
    rig->easing_provider = NM89_INVALID_ID;
    rig->clock_provider = NM89_INVALID_ID;
    rig->player.clip_id = NM89_INVALID_ID;
    rig->player.queued_clip = NM89_INVALID_ID;
    rig->player.tick = 0U;
    rig->player.previous_tick = 0U;
    rig->player.playing = 0U;
    rig->player.reverse = 0U;

    for (i = 0; i < NM89_MAX_PARTS; ++i) {
        rig->parts[i].name[0] = '\0';
        nm89_part_provider_defaults(&rig->parts[i]);
    }
    for (i = 0; i < NM89_MAX_PROVIDERS; ++i) {
        rig->providers[i].user = 0;
        rig->providers[i].sample_transform = 0;
        rig->providers[i].sample_socket = 0;
        rig->providers[i].sample_pivot = 0;
        rig->providers[i].sample_world = 0;
        rig->providers[i].sample_visibility = 0;
        rig->providers[i].sample_constraint = 0;
        rig->providers[i].resolve_geometry = 0;
        rig->providers[i].apply_geometry = 0;
        rig->providers[i].emit_event = 0;
        rig->providers[i].sample_delta_ticks = 0;
        rig->providers[i].log = 0;
        rig->providers[i].matrix_identity = 0;
        rig->providers[i].matrix_multiply = 0;
        rig->providers[i].matrix_from_transform = 0;
        rig->providers[i].matrix_transform_point = 0;
        rig->providers[i].ease = 0;
    }
}

int nm89_provider_add(nm89_rig *rig, const nm89_provider *provider,
                      nm89_i16 *out_provider_id)
{
    nm89_i16 id;
    if (rig == 0 || provider == 0) {
        return NM89_ERR_ARGUMENT;
    }
    if (rig->provider_count >= NM89_MAX_PROVIDERS) {
        return NM89_ERR_CAPACITY;
    }
    id = (nm89_i16)rig->provider_count;
    rig->providers[id] = *provider;
    rig->provider_count += 1U;
    if (out_provider_id != 0) {
        *out_provider_id = id;
    }
    return NM89_OK;
}

static int nm89_set_provider_selector(nm89_rig *rig,
                                      nm89_i16 provider_id,
                                      nm89_i16 *destination)
{
    if (rig == 0 || destination == 0) {
        return NM89_ERR_ARGUMENT;
    }
    if (provider_id != NM89_INVALID_ID &&
        !nm89_valid_provider(rig, provider_id)) {
        return NM89_ERR_PROVIDER;
    }
    *destination = provider_id;
    return NM89_OK;
}

int nm89_rig_set_math_provider(nm89_rig *rig, nm89_i16 provider_id)
{
    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    return nm89_set_provider_selector(rig, provider_id,
                                      &rig->math_provider);
}

int nm89_rig_set_easing_provider(nm89_rig *rig, nm89_i16 provider_id)
{
    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    return nm89_set_provider_selector(rig, provider_id,
                                      &rig->easing_provider);
}

int nm89_rig_set_clock_provider(nm89_rig *rig, nm89_i16 provider_id)
{
    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    return nm89_set_provider_selector(rig, provider_id,
                                      &rig->clock_provider);
}

int nm89_constraint_add(nm89_rig *rig,
                        const nm89_constraint *constraint_value,
                        nm89_i16 *out_constraint_id)
{
    nm89_i16 id;
    if (rig == 0 || constraint_value == 0) {
        return NM89_ERR_ARGUMENT;
    }
    if (rig->constraint_count >= NM89_MAX_CONSTRAINTS) {
        return NM89_ERR_CAPACITY;
    }
    id = (nm89_i16)rig->constraint_count;
    rig->constraints[id] = *constraint_value;
    rig->constraint_count += 1U;
    if (out_constraint_id != 0) {
        *out_constraint_id = id;
    }
    return NM89_OK;
}

int nm89_pivot_add(nm89_rig *rig, const nm89_pivot *pivot,
                   nm89_i16 *out_pivot_id)
{
    nm89_i16 id;
    if (rig == 0 || pivot == 0) {
        return NM89_ERR_ARGUMENT;
    }
    if (rig->pivot_count >= NM89_MAX_PIVOTS) {
        return NM89_ERR_CAPACITY;
    }
    id = (nm89_i16)rig->pivot_count;
    rig->pivots[id] = *pivot;
    rig->pivot_count += 1U;
    if (out_pivot_id != 0) {
        *out_pivot_id = id;
    }
    return NM89_OK;
}

int nm89_alignment_add(nm89_rig *rig, const nm89_alignment *alignment,
                       nm89_i16 *out_alignment_id)
{
    nm89_i16 id;
    if (rig == 0 || alignment == 0) {
        return NM89_ERR_ARGUMENT;
    }
    if (rig->alignment_count >= NM89_MAX_ALIGNMENTS) {
        return NM89_ERR_CAPACITY;
    }
    id = (nm89_i16)rig->alignment_count;
    rig->alignments[id] = *alignment;
    rig->alignment_count += 1U;
    if (out_alignment_id != 0) {
        *out_alignment_id = id;
    }
    return NM89_OK;
}

int nm89_part_add(nm89_rig *rig, const char *name,
                  nm89_i16 parent_part, nm89_i16 user_tag,
                  const nm89_transform *home,
                  nm89_i16 constraint_id, nm89_i16 pivot_id,
                  nm89_u8 visible, nm89_i16 *out_part_id)
{
    nm89_part *part;
    nm89_i16 id;
    nm89_transform identity;
    const nm89_transform *initial;

    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    if (rig->part_count >= NM89_MAX_PARTS) {
        return NM89_ERR_CAPACITY;
    }
    if (parent_part != NM89_ROOT_PART &&
        !nm89_valid_part(rig, parent_part)) {
        return NM89_ERR_HIERARCHY;
    }
    if (constraint_id != NM89_INVALID_ID &&
        !nm89_valid_constraint(rig, constraint_id)) {
        return NM89_ERR_RANGE;
    }
    if (pivot_id != NM89_INVALID_ID &&
        !nm89_valid_pivot(rig, pivot_id)) {
        return NM89_ERR_RANGE;
    }

    nm89_transform_identity(&identity);
    initial = home != 0 ? home : &identity;
    id = (nm89_i16)rig->part_count;
    part = &rig->parts[id];
    part->parent_part = parent_part;
    part->constraint_id = constraint_id;
    part->pivot_id = pivot_id;
    part->user_tag = user_tag;
    part->flags = NM89_PART_ENABLED;
    if (visible) {
        part->flags |= NM89_PART_VISIBLE;
    }
    nm89_copy_name(part->name, name);
    part->home = *initial;
    part->manual = *initial;
    part->pose.requested = *initial;
    part->pose.resolved = *initial;
    part->pose.requested_visible = visible ? 1U : 0U;
    part->pose.resolved_visible = visible ? 1U : 0U;
    part->world_visible = visible ? 1U : 0U;
    nm89_matrix_identity(&part->world);
    nm89_part_provider_defaults(part);

    rig->part_count += 1U;
    if (out_part_id != 0) {
        *out_part_id = id;
    }
    return NM89_OK;
}

int nm89_part_find(const nm89_rig *rig, const char *name)
{
    nm89_u16 i;
    if (rig == 0 || name == 0) {
        return NM89_ERR_ARGUMENT;
    }
    for (i = 0; i < rig->part_count; ++i) {
        if (nm89_names_equal(rig->parts[i].name, name)) {
            return (int)i;
        }
    }
    return NM89_ERR_NOT_FOUND;
}

int nm89_part_set_parent(nm89_rig *rig, nm89_i16 part_id,
                         nm89_i16 parent_part)
{
    nm89_i16 cursor;
    if (!nm89_valid_part(rig, part_id)) {
        return NM89_ERR_ARGUMENT;
    }
    if (parent_part != NM89_ROOT_PART &&
        !nm89_valid_part(rig, parent_part)) {
        return NM89_ERR_HIERARCHY;
    }
    cursor = parent_part;
    while (cursor != NM89_ROOT_PART) {
        if (cursor == part_id) {
            return NM89_ERR_HIERARCHY;
        }
        cursor = rig->parts[cursor].parent_part;
    }
    rig->parts[part_id].parent_part = parent_part;
    return NM89_OK;
}

int nm89_part_set_manual_transform(nm89_rig *rig, nm89_i16 part_id,
                                   const nm89_transform *transform_value)
{
    if (!nm89_valid_part(rig, part_id) || transform_value == 0) {
        return NM89_ERR_ARGUMENT;
    }
    rig->parts[part_id].manual = *transform_value;
    return NM89_OK;
}

int nm89_part_set_channel(nm89_rig *rig, nm89_i16 part_id,
                          int property, nm89_fx value)
{
    nm89_fx *channel;
    if (!nm89_valid_part(rig, part_id) ||
        property < NM89_PROP_MOVE_X ||
        property > NM89_PROP_SCALE_Z) {
        return NM89_ERR_ARGUMENT;
    }
    channel = nm89_transform_channel(&rig->parts[part_id].manual,
                                     property);
    *channel = value;
    return NM89_OK;
}

int nm89_part_set_visible(nm89_rig *rig, nm89_i16 part_id,
                          nm89_u8 visible)
{
    if (!nm89_valid_part(rig, part_id)) {
        return NM89_ERR_ARGUMENT;
    }
    if (visible) {
        rig->parts[part_id].flags |= NM89_PART_VISIBLE;
    } else {
        rig->parts[part_id].flags &= (nm89_u16)~NM89_PART_VISIBLE;
    }
    return NM89_OK;
}

int nm89_part_set_enabled(nm89_rig *rig, nm89_i16 part_id,
                          nm89_u8 enabled)
{
    if (!nm89_valid_part(rig, part_id)) {
        return NM89_ERR_ARGUMENT;
    }
    if (enabled) {
        rig->parts[part_id].flags |= NM89_PART_ENABLED;
    } else {
        rig->parts[part_id].flags &= (nm89_u16)~NM89_PART_ENABLED;
    }
    return NM89_OK;
}

void nm89_rig_reset_manual_pose(nm89_rig *rig)
{
    nm89_u16 i;
    if (rig == 0) {
        return;
    }
    for (i = 0; i < rig->part_count; ++i) {
        rig->parts[i].manual = rig->parts[i].home;
    }
}

int nm89_part_bind_transform_provider(nm89_rig *rig, nm89_i16 part_id,
                                      nm89_i16 provider_id,
                                      nm89_i16 source_id,
                                      unsigned long channels,
                                      nm89_u8 mode)
{
    nm89_provider *provider;
    if (!nm89_valid_part(rig, part_id) ||
        (mode != NM89_PROVIDER_REPLACE &&
         mode != NM89_PROVIDER_ADDITIVE)) {
        return NM89_ERR_ARGUMENT;
    }
    provider = nm89_get_provider(rig, provider_id);
    if (provider == 0 || provider->sample_transform == 0) {
        return NM89_ERR_PROVIDER;
    }
    rig->parts[part_id].transform_provider = provider_id;
    rig->parts[part_id].transform_source = source_id;
    rig->parts[part_id].transform_channels = channels;
    rig->parts[part_id].transform_mode = mode;
    return NM89_OK;
}

int nm89_part_bind_socket_provider(nm89_rig *rig, nm89_i16 part_id,
                                   nm89_i16 provider_id,
                                   nm89_i16 socket_id,
                                   nm89_u8 socket_space)
{
    nm89_provider *provider;
    if (!nm89_valid_part(rig, part_id) ||
        (socket_space != NM89_SOCKET_PARENT_SPACE &&
         socket_space != NM89_SOCKET_WORLD_SPACE)) {
        return NM89_ERR_ARGUMENT;
    }
    provider = nm89_get_provider(rig, provider_id);
    if (provider == 0 || provider->sample_socket == 0) {
        return NM89_ERR_PROVIDER;
    }
    rig->parts[part_id].socket_provider = provider_id;
    rig->parts[part_id].socket_source = socket_id;
    rig->parts[part_id].socket_space = socket_space;
    return NM89_OK;
}

int nm89_part_bind_pivot_provider(nm89_rig *rig, nm89_i16 part_id,
                                  nm89_i16 provider_id,
                                  nm89_i16 source_id)
{
    nm89_provider *provider;
    if (!nm89_valid_part(rig, part_id)) {
        return NM89_ERR_ARGUMENT;
    }
    provider = nm89_get_provider(rig, provider_id);
    if (provider == 0 || provider->sample_pivot == 0) {
        return NM89_ERR_PROVIDER;
    }
    rig->parts[part_id].pivot_provider = provider_id;
    rig->parts[part_id].pivot_source = source_id;
    return NM89_OK;
}

int nm89_part_bind_world_provider(nm89_rig *rig, nm89_i16 part_id,
                                  nm89_i16 provider_id,
                                  nm89_i16 source_id,
                                  nm89_u8 world_mode)
{
    nm89_provider *provider;
    if (!nm89_valid_part(rig, part_id) ||
        (world_mode != NM89_WORLD_PREMULTIPLY &&
         world_mode != NM89_WORLD_REPLACE)) {
        return NM89_ERR_ARGUMENT;
    }
    provider = nm89_get_provider(rig, provider_id);
    if (provider == 0 || provider->sample_world == 0) {
        return NM89_ERR_PROVIDER;
    }
    rig->parts[part_id].world_provider = provider_id;
    rig->parts[part_id].world_source = source_id;
    rig->parts[part_id].world_mode = world_mode;
    return NM89_OK;
}

int nm89_part_bind_visibility_provider(nm89_rig *rig, nm89_i16 part_id,
                                       nm89_i16 provider_id,
                                       nm89_i16 source_id,
                                       nm89_u8 visibility_mode)
{
    nm89_provider *provider;
    if (!nm89_valid_part(rig, part_id) ||
        (visibility_mode != NM89_VISIBILITY_AND &&
         visibility_mode != NM89_VISIBILITY_REPLACE)) {
        return NM89_ERR_ARGUMENT;
    }
    provider = nm89_get_provider(rig, provider_id);
    if (provider == 0 || provider->sample_visibility == 0) {
        return NM89_ERR_PROVIDER;
    }
    rig->parts[part_id].visibility_provider = provider_id;
    rig->parts[part_id].visibility_source = source_id;
    rig->parts[part_id].visibility_mode = visibility_mode;
    return NM89_OK;
}

int nm89_part_bind_constraint_provider(nm89_rig *rig, nm89_i16 part_id,
                                       nm89_i16 provider_id,
                                       nm89_i16 source_id,
                                       nm89_u8 constraint_mode)
{
    nm89_provider *provider;
    if (!nm89_valid_part(rig, part_id) ||
        (constraint_mode != NM89_DYNAMIC_CONSTRAINT_INTERSECT &&
         constraint_mode != NM89_DYNAMIC_CONSTRAINT_REPLACE)) {
        return NM89_ERR_ARGUMENT;
    }
    provider = nm89_get_provider(rig, provider_id);
    if (provider == 0 || provider->sample_constraint == 0) {
        return NM89_ERR_PROVIDER;
    }
    rig->parts[part_id].constraint_provider = provider_id;
    rig->parts[part_id].constraint_source = source_id;
    rig->parts[part_id].dynamic_constraint_mode = constraint_mode;
    return NM89_OK;
}

int nm89_part_clear_provider_bindings(nm89_rig *rig, nm89_i16 part_id)
{
    if (!nm89_valid_part(rig, part_id)) {
        return NM89_ERR_ARGUMENT;
    }
    nm89_part_provider_defaults(&rig->parts[part_id]);
    return NM89_OK;
}

int nm89_binding_add(nm89_rig *rig, nm89_i16 owner_part,
                     nm89_i16 resource_id, nm89_u8 selector_type,
                     nm89_i16 selector_id, nm89_i16 mesh_id,
                     nm89_i16 alignment_id,
                     nm89_i16 resolver_provider,
                     nm89_i16 user_tag,
                     nm89_i16 *out_binding_id)
{
    nm89_binding *binding;
    nm89_i16 id;
    if (!nm89_valid_part(rig, owner_part)) {
        return NM89_ERR_ARGUMENT;
    }
    if (rig->binding_count >= NM89_MAX_BINDINGS) {
        return NM89_ERR_CAPACITY;
    }
    if (selector_type > NM89_SELECTOR_PROCEDURAL) {
        return NM89_ERR_RANGE;
    }
    if (alignment_id != NM89_INVALID_ID &&
        !nm89_valid_alignment(rig, alignment_id)) {
        return NM89_ERR_RANGE;
    }
    if (resolver_provider != NM89_INVALID_ID) {
        if (!nm89_valid_provider(rig, resolver_provider) ||
            rig->providers[resolver_provider].resolve_geometry == 0) {
            return NM89_ERR_PROVIDER;
        }
    }
    id = (nm89_i16)rig->binding_count;
    binding = &rig->bindings[id];
    binding->owner_part = owner_part;
    binding->resource_id = resource_id;
    binding->selector_id = selector_id;
    binding->mesh_id = mesh_id;
    binding->alignment_id = alignment_id;
    binding->resolver_provider = resolver_provider;
    binding->user_tag = user_tag;
    binding->selector_type = selector_type;
    binding->flags = NM89_BINDING_VISIBLE;
    rig->binding_count += 1U;
    if (out_binding_id != 0) {
        *out_binding_id = id;
    }
    return NM89_OK;
}

int nm89_binding_set_visible(nm89_rig *rig, nm89_i16 binding_id,
                             nm89_u8 visible)
{
    if (rig == 0 || binding_id < 0 ||
        (nm89_u16)binding_id >= rig->binding_count) {
        return NM89_ERR_ARGUMENT;
    }
    if (visible) {
        rig->bindings[binding_id].flags |= NM89_BINDING_VISIBLE;
    } else {
        rig->bindings[binding_id].flags &=
            (nm89_u8)~NM89_BINDING_VISIBLE;
    }
    return NM89_OK;
}

int nm89_binding_set_resolver(nm89_rig *rig, nm89_i16 binding_id,
                              nm89_i16 provider_id)
{
    if (rig == 0 || binding_id < 0 ||
        (nm89_u16)binding_id >= rig->binding_count) {
        return NM89_ERR_ARGUMENT;
    }
    if (provider_id != NM89_INVALID_ID &&
        (!nm89_valid_provider(rig, provider_id) ||
         rig->providers[provider_id].resolve_geometry == 0)) {
        return NM89_ERR_PROVIDER;
    }
    rig->bindings[binding_id].resolver_provider = provider_id;
    return NM89_OK;
}

int nm89_clip_begin(nm89_rig *rig, nm89_i16 user_tag,
                    nm89_u16 length_ticks, nm89_u8 loop,
                    nm89_i16 *out_clip_id)
{
    nm89_clip *clip;
    nm89_i16 id;
    if (rig == 0 || length_ticks == 0U) {
        return NM89_ERR_ARGUMENT;
    }
    if (rig->builder_clip != NM89_INVALID_ID) {
        return NM89_ERR_STATE;
    }
    if (rig->clip_count >= NM89_MAX_CLIPS) {
        return NM89_ERR_CAPACITY;
    }
    id = (nm89_i16)rig->clip_count;
    clip = &rig->clips[id];
    clip->first_track = rig->track_count;
    clip->track_count = 0U;
    clip->first_event = rig->event_count;
    clip->event_count = 0U;
    clip->length_ticks = length_ticks;
    clip->user_tag = user_tag;
    clip->flags = NM89_CLIP_ENABLED;
    if (loop) {
        clip->flags |= NM89_CLIP_LOOP;
    }
    rig->clip_count += 1U;
    rig->builder_clip = id;
    rig->builder_track = NM89_INVALID_ID;
    if (out_clip_id != 0) {
        *out_clip_id = id;
    }
    return NM89_OK;
}

int nm89_clip_add_track(nm89_rig *rig, nm89_i16 part_id,
                        nm89_u8 property, nm89_u8 interpolation,
                        nm89_i16 *out_track_id)
{
    nm89_track *track;
    nm89_clip *clip;
    nm89_i16 id;
    if (rig == 0 || rig->builder_clip == NM89_INVALID_ID ||
        !nm89_valid_part(rig, part_id)) {
        return NM89_ERR_STATE;
    }
    if (property >= NM89_PROPERTY_COUNT ||
        interpolation > NM89_INTERP_CUSTOM) {
        return NM89_ERR_RANGE;
    }
    if (rig->track_count >= NM89_MAX_TRACKS) {
        return NM89_ERR_CAPACITY;
    }
    id = (nm89_i16)rig->track_count;
    track = &rig->tracks[id];
    track->part_id = part_id;
    track->first_key = rig->key_count;
    track->key_count = 0U;
    track->property = property;
    track->interpolation = interpolation;
    rig->track_count += 1U;
    clip = &rig->clips[rig->builder_clip];
    clip->track_count += 1U;
    rig->builder_track = id;
    if (out_track_id != 0) {
        *out_track_id = id;
    }
    return NM89_OK;
}

int nm89_track_add_key(nm89_rig *rig, nm89_i16 track_id,
                       nm89_u16 tick, nm89_fx value)
{
    nm89_track *track;
    nm89_key *key;
    nm89_clip *clip;
    if (rig == 0 || rig->builder_clip == NM89_INVALID_ID ||
        track_id != rig->builder_track ||
        track_id < 0 || (nm89_u16)track_id >= rig->track_count) {
        return NM89_ERR_STATE;
    }
    if (rig->key_count >= NM89_MAX_KEYS) {
        return NM89_ERR_CAPACITY;
    }
    clip = &rig->clips[rig->builder_clip];
    if (tick > clip->length_ticks) {
        return NM89_ERR_RANGE;
    }
    track = &rig->tracks[track_id];
    if (track->key_count > 0U) {
        key = &rig->keys[track->first_key + track->key_count - 1U];
        if (tick < key->tick) {
            return NM89_ERR_RANGE;
        }
    }
    key = &rig->keys[rig->key_count];
    key->tick = tick;
    key->value = value;
    rig->key_count += 1U;
    track->key_count += 1U;
    return NM89_OK;
}

int nm89_clip_add_event(nm89_rig *rig,
                        const nm89_clip_event *event_value)
{
    nm89_clip *clip;
    if (rig == 0 || event_value == 0 ||
        rig->builder_clip == NM89_INVALID_ID) {
        return NM89_ERR_STATE;
    }
    if (rig->event_count >= NM89_MAX_EVENTS) {
        return NM89_ERR_CAPACITY;
    }
    clip = &rig->clips[rig->builder_clip];
    if (event_value->tick > clip->length_ticks) {
        return NM89_ERR_RANGE;
    }
    if (event_value->part_id != NM89_INVALID_ID &&
        !nm89_valid_part(rig, event_value->part_id)) {
        return NM89_ERR_RANGE;
    }
    rig->events[rig->event_count] = *event_value;
    rig->event_count += 1U;
    clip->event_count += 1U;
    return NM89_OK;
}

int nm89_clip_end(nm89_rig *rig)
{
    if (rig == 0 || rig->builder_clip == NM89_INVALID_ID) {
        return NM89_ERR_STATE;
    }
    rig->builder_clip = NM89_INVALID_ID;
    rig->builder_track = NM89_INVALID_ID;
    return NM89_OK;
}

int nm89_action_bind(nm89_rig *rig, nm89_i16 action_id,
                     nm89_i16 clip_id, nm89_u8 policy)
{
    nm89_u16 i;
    nm89_action *action;
    if (rig == 0 || clip_id < 0 ||
        (nm89_u16)clip_id >= rig->clip_count ||
        policy > NM89_ACTION_QUEUE) {
        return NM89_ERR_ARGUMENT;
    }
    for (i = 0; i < rig->action_count; ++i) {
        if (rig->actions[i].action_id == action_id) {
            rig->actions[i].clip_id = clip_id;
            rig->actions[i].policy = policy;
            rig->actions[i].enabled = 1U;
            return NM89_OK;
        }
    }
    if (rig->action_count >= NM89_MAX_ACTIONS) {
        return NM89_ERR_CAPACITY;
    }
    action = &rig->actions[rig->action_count];
    action->action_id = action_id;
    action->clip_id = clip_id;
    action->policy = policy;
    action->enabled = 1U;
    rig->action_count += 1U;
    return NM89_OK;
}

int nm89_play(nm89_rig *rig, nm89_i16 clip_id, nm89_u8 restart)
{
    nm89_clip *clip;
    if (rig == 0 || clip_id < 0 ||
        (nm89_u16)clip_id >= rig->clip_count) {
        return NM89_ERR_ARGUMENT;
    }
    clip = &rig->clips[clip_id];
    if ((clip->flags & NM89_CLIP_ENABLED) == 0U) {
        return NM89_ERR_STATE;
    }
    if (rig->player.playing && rig->player.clip_id == clip_id &&
        !restart) {
        return NM89_OK;
    }
    rig->player.clip_id = clip_id;
    rig->player.queued_clip = NM89_INVALID_ID;
    rig->player.tick = 0U;
    rig->player.previous_tick = 0U;
    rig->player.playing = 1U;
    rig->player.reverse = 0U;
    return NM89_OK;
}

void nm89_stop(nm89_rig *rig)
{
    if (rig == 0) {
        return;
    }
    rig->player.playing = 0U;
    rig->player.queued_clip = NM89_INVALID_ID;
}

int nm89_seek(nm89_rig *rig, nm89_u16 tick)
{
    nm89_clip *clip;
    if (rig == 0 || rig->player.clip_id == NM89_INVALID_ID) {
        return NM89_ERR_STATE;
    }
    clip = &rig->clips[rig->player.clip_id];
    if (tick > clip->length_ticks) {
        return NM89_ERR_RANGE;
    }
    rig->player.previous_tick = rig->player.tick;
    rig->player.tick = tick;
    return NM89_OK;
}

int nm89_trigger_action(nm89_rig *rig, nm89_i16 action_id)
{
    nm89_u16 i;
    nm89_action *action;
    nm89_clip *clip;
    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    for (i = 0; i < rig->action_count; ++i) {
        action = &rig->actions[i];
        if (!action->enabled || action->action_id != action_id) {
            continue;
        }
        if (action->policy == NM89_ACTION_RESTART) {
            return nm89_play(rig, action->clip_id, 1U);
        }
        if (action->policy == NM89_ACTION_IGNORE_IF_PLAYING) {
            if (rig->player.playing) {
                return NM89_OK;
            }
            return nm89_play(rig, action->clip_id, 1U);
        }
        if (action->policy == NM89_ACTION_REVERSE) {
            if (rig->player.playing &&
                rig->player.clip_id == action->clip_id) {
                rig->player.reverse = rig->player.reverse ? 0U : 1U;
                return NM89_OK;
            }
            clip = &rig->clips[action->clip_id];
            rig->player.clip_id = action->clip_id;
            rig->player.tick = clip->length_ticks;
            rig->player.previous_tick = rig->player.tick;
            rig->player.playing = 1U;
            rig->player.reverse = 1U;
            return NM89_OK;
        }
        if (action->policy == NM89_ACTION_QUEUE) {
            if (rig->player.playing) {
                rig->player.queued_clip = action->clip_id;
                return NM89_OK;
            }
            return nm89_play(rig, action->clip_id, 1U);
        }
    }
    return NM89_ERR_NOT_FOUND;
}

static nm89_fx nm89_apply_axis_constraint(
    const nm89_axis_constraint *axis, nm89_fx requested)
{
    nm89_fx value;
    nm89_fx range;
    nm89_fx offset;
    nm89_i32 steps;
    if (axis == 0 || axis->mode == NM89_CONSTRAINT_FREE) {
        return requested;
    }
    if (axis->mode == NM89_CONSTRAINT_LOCKED) {
        return axis->minimum;
    }
    if (axis->maximum < axis->minimum) {
        return requested;
    }
    value = requested;
    if (axis->mode == NM89_CONSTRAINT_WRAPPED) {
        range = axis->maximum - axis->minimum;
        if (range <= 0) {
            return axis->minimum;
        }
        while (value > axis->maximum) {
            value -= range;
        }
        while (value < axis->minimum) {
            value += range;
        }
        return value;
    }
    if (value < axis->minimum) {
        value = axis->minimum;
    }
    if (value > axis->maximum) {
        value = axis->maximum;
    }
    if (axis->mode == NM89_CONSTRAINT_STEPPED && axis->step > 0) {
        offset = value - axis->minimum;
        steps = offset / axis->step;
        value = axis->minimum + steps * axis->step;
    }
    return value;
}

static void nm89_apply_sample(nm89_transform *destination,
                              nm89_u8 *destination_visible,
                              const nm89_transform_sample *sample,
                              unsigned long allowed_channels,
                              nm89_u8 mode)
{
    int property;
    unsigned long channel_mask;
    nm89_fx *output;
    const nm89_fx *input;
    if (destination == 0 || destination_visible == 0 || sample == 0) {
        return;
    }
    for (property = NM89_PROP_MOVE_X;
         property <= NM89_PROP_SCALE_Z;
         ++property) {
        channel_mask = 1UL << property;
        if ((allowed_channels & sample->channels & channel_mask) == 0UL) {
            continue;
        }
        output = nm89_transform_channel(destination, property);
        input = nm89_transform_channel_const(&sample->transform, property);
        if (mode == NM89_PROVIDER_ADDITIVE) {
            if (property >= NM89_PROP_SCALE_X) {
                *output = nm89_mul(*output, *input);
            } else {
                *output += *input;
            }
        } else {
            *output = *input;
        }
    }
    if (sample->has_visibility &&
        (allowed_channels & NM89_CHANNEL_VISIBLE) != 0UL) {
        if (mode == NM89_PROVIDER_ADDITIVE) {
            *destination_visible =
                (*destination_visible && sample->visible) ? 1U : 0U;
        } else {
            *destination_visible = sample->visible ? 1U : 0U;
        }
    }
}

static nm89_fx nm89_builtin_ease(nm89_u8 interpolation, nm89_fx t)
{
    nm89_fx inverse;
    nm89_fx result;
    if (t < 0) {
        t = 0;
    }
    if (t > NM89_FX_ONE) {
        t = NM89_FX_ONE;
    }
    if (interpolation == NM89_INTERP_STEP) {
        return 0;
    }
    if (interpolation == NM89_INTERP_LINEAR) {
        return t;
    }
    if (interpolation == NM89_INTERP_EASE_IN) {
        return nm89_mul(t, t);
    }
    if (interpolation == NM89_INTERP_EASE_OUT) {
        inverse = NM89_FX_ONE - t;
        return NM89_FX_ONE - nm89_mul(inverse, inverse);
    }
    if (interpolation == NM89_INTERP_SMOOTH) {
        result = nm89_mul(t, t);
        return nm89_mul(result,
                        NM89_FX_FROM_INT(3) -
                        nm89_mul(NM89_FX_FROM_INT(2), t));
    }
    return t;
}

static nm89_fx nm89_ease_value(const nm89_rig *rig,
                               nm89_u8 interpolation, nm89_fx t)
{
    const nm89_provider *provider;
    if (interpolation == NM89_INTERP_CUSTOM) {
        provider = nm89_get_provider_const(rig, rig->easing_provider);
        if (provider != 0 && provider->ease != 0) {
            return provider->ease(provider->user, interpolation, t);
        }
        return t;
    }
    return nm89_builtin_ease(interpolation, t);
}

static nm89_fx nm89_track_evaluate(const nm89_rig *rig,
                                   const nm89_track *track,
                                   nm89_u16 tick)
{
    const nm89_key *first;
    const nm89_key *last;
    const nm89_key *a;
    const nm89_key *b;
    nm89_u16 i;
    nm89_fx t;
    nm89_fx eased;
    nm89_i32 span;
    nm89_i32 offset;

    if (rig == 0 || track == 0 || track->key_count == 0U) {
        return 0;
    }
    first = &rig->keys[track->first_key];
    last = &rig->keys[track->first_key + track->key_count - 1U];
    if (tick <= first->tick) {
        return first->value;
    }
    if (tick >= last->tick) {
        return last->value;
    }
    for (i = 0U; i + 1U < track->key_count; ++i) {
        a = &rig->keys[track->first_key + i];
        b = &rig->keys[track->first_key + i + 1U];
        if (tick >= a->tick && tick <= b->tick) {
            if (track->interpolation == NM89_INTERP_STEP ||
                b->tick == a->tick) {
                return a->value;
            }
            span = (nm89_i32)b->tick - (nm89_i32)a->tick;
            offset = (nm89_i32)tick - (nm89_i32)a->tick;
            t = nm89_fx_from_ratio(offset, span);
            eased = nm89_ease_value(rig, track->interpolation, t);
            return nm89_lerp(a->value, b->value, eased);
        }
    }
    return last->value;
}

static void nm89_apply_active_clip(nm89_rig *rig)
{
    nm89_clip *clip;
    nm89_track *track;
    nm89_part *part;
    nm89_fx value;
    nm89_fx *channel;
    nm89_u16 i;
    if (rig == 0 || rig->player.clip_id == NM89_INVALID_ID ||
        (nm89_u16)rig->player.clip_id >= rig->clip_count) {
        return;
    }
    clip = &rig->clips[rig->player.clip_id];
    for (i = 0U; i < clip->track_count; ++i) {
        track = &rig->tracks[clip->first_track + i];
        if (!nm89_valid_part(rig, track->part_id) ||
            track->key_count == 0U) {
            continue;
        }
        part = &rig->parts[track->part_id];
        value = nm89_track_evaluate(rig, track, rig->player.tick);
        if (track->property == NM89_PROP_VISIBLE) {
            part->pose.requested_visible = value != 0 ? 1U : 0U;
        } else {
            channel = nm89_transform_channel(&part->pose.requested,
                                             track->property);
            if (channel != 0) {
                *channel = value;
            }
        }
    }
}

static void nm89_resolve_part_pose(nm89_rig *rig, nm89_i16 part_id)
{
    nm89_part *part;
    const nm89_constraint *static_constraint;
    nm89_constraint dynamic_constraint;
    int has_dynamic;
    nm89_provider *provider;
    nm89_u8 provider_visible;
    int property;
    nm89_fx *resolved_channel;
    const nm89_fx *requested_channel;
    const nm89_axis_constraint *axis;
    int result;

    part = &rig->parts[part_id];
    part->pose.resolved = part->pose.requested;
    part->pose.resolved_visible = part->pose.requested_visible;
    static_constraint = 0;
    if (nm89_valid_constraint(rig, part->constraint_id)) {
        static_constraint = &rig->constraints[part->constraint_id];
    }
    has_dynamic = 0;
    provider = nm89_get_provider(rig, part->constraint_provider);
    if (provider != 0 && provider->sample_constraint != 0) {
        nm89_constraint_free(&dynamic_constraint);
        result = provider->sample_constraint(provider->user, rig,
                                             part_id,
                                             part->constraint_source,
                                             &dynamic_constraint);
        if (result == NM89_OK) {
            has_dynamic = 1;
        } else {
            nm89_log_all(rig, NM89_LOG_WARNING, result,
                         "constraint provider sample failed");
        }
    }

    for (property = NM89_PROP_MOVE_X;
         property <= NM89_PROP_SCALE_Z;
         ++property) {
        requested_channel = nm89_transform_channel_const(
            &part->pose.requested, property);
        resolved_channel = nm89_transform_channel(
            &part->pose.resolved, property);
        *resolved_channel = *requested_channel;
        if (static_constraint != 0 &&
            !(has_dynamic &&
              part->dynamic_constraint_mode ==
              NM89_DYNAMIC_CONSTRAINT_REPLACE)) {
            axis = nm89_constraint_axis_const(static_constraint,
                                              property);
            *resolved_channel = nm89_apply_axis_constraint(
                axis, *resolved_channel);
        }
        if (has_dynamic) {
            axis = nm89_constraint_axis_const(&dynamic_constraint,
                                              property);
            *resolved_channel = nm89_apply_axis_constraint(
                axis, *resolved_channel);
        }
    }

    provider = nm89_get_provider(rig, part->visibility_provider);
    if (provider != 0 && provider->sample_visibility != 0) {
        provider_visible = 1U;
        result = provider->sample_visibility(
            provider->user, rig, part_id, NM89_INVALID_ID,
            part->visibility_source, &provider_visible);
        if (result == NM89_OK) {
            if (part->visibility_mode == NM89_VISIBILITY_REPLACE) {
                part->pose.resolved_visible =
                    provider_visible ? 1U : 0U;
            } else {
                part->pose.resolved_visible =
                    (part->pose.resolved_visible && provider_visible) ?
                    1U : 0U;
            }
        } else {
            nm89_log_all(rig, NM89_LOG_WARNING, result,
                         "visibility provider sample failed");
        }
    }

}

int nm89_evaluate(nm89_rig *rig)
{
    nm89_u16 i;
    nm89_part *part;
    nm89_provider *provider;
    nm89_transform_sample sample;
    int result;
    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    for (i = 0U; i < rig->part_count; ++i) {
        part = &rig->parts[i];
        part->pose.requested = part->manual;
        part->pose.requested_visible =
            (part->flags & NM89_PART_VISIBLE) != 0U ? 1U : 0U;
        if ((part->flags & NM89_PART_ENABLED) == 0U) {
            part->pose.requested_visible = 0U;
        }
    }

    nm89_apply_active_clip(rig);

    for (i = 0U; i < rig->part_count; ++i) {
        part = &rig->parts[i];
        provider = nm89_get_provider(rig, part->transform_provider);
        if (provider != 0 && provider->sample_transform != 0) {
            nm89_transform_sample_identity(&sample);
            result = provider->sample_transform(
                provider->user, rig, (nm89_i16)i,
                part->transform_source, &sample);
            if (result == NM89_OK) {
                nm89_apply_sample(&part->pose.requested,
                                  &part->pose.requested_visible,
                                  &sample,
                                  part->transform_channels,
                                  part->transform_mode);
            } else {
                nm89_log_all(rig, NM89_LOG_WARNING, result,
                             "transform provider sample failed");
            }
        }
        nm89_resolve_part_pose(rig, (nm89_i16)i);
    }
    return NM89_OK;
}

static void nm89_emit_event(nm89_rig *rig,
                            const nm89_clip_event *event_value)
{
    nm89_u16 i;
    for (i = 0U; i < rig->provider_count; ++i) {
        if (rig->providers[i].emit_event != 0) {
            rig->providers[i].emit_event(rig->providers[i].user,
                                         rig, event_value);
        }
    }
}

static void nm89_emit_events_range(nm89_rig *rig,
                                   const nm89_clip *clip,
                                   nm89_u16 from_tick,
                                   nm89_u16 to_tick,
                                   nm89_u8 reverse)
{
    nm89_u16 i;
    const nm89_clip_event *event_value;
    for (i = 0U; i < clip->event_count; ++i) {
        event_value = &rig->events[clip->first_event + i];
        if (!reverse) {
            if (event_value->tick > from_tick &&
                event_value->tick <= to_tick) {
                nm89_emit_event(rig, event_value);
            }
        } else {
            if (event_value->tick < from_tick &&
                event_value->tick >= to_tick) {
                nm89_emit_event(rig, event_value);
            }
        }
    }
}

static void nm89_finish_or_queue(nm89_rig *rig)
{
    nm89_i16 queued;
    queued = rig->player.queued_clip;
    rig->player.playing = 0U;
    rig->player.queued_clip = NM89_INVALID_ID;
    if (queued != NM89_INVALID_ID) {
        nm89_play(rig, queued, 1U);
    }
}

static void nm89_advance_player(nm89_rig *rig, nm89_u16 delta_ticks)
{
    nm89_clip *clip;
    nm89_u32 remaining;
    nm89_u32 room;
    nm89_u16 old_tick;
    nm89_u16 new_tick;
    nm89_u8 loop;

    if (!rig->player.playing ||
        rig->player.clip_id == NM89_INVALID_ID) {
        return;
    }
    clip = &rig->clips[rig->player.clip_id];
    loop = (clip->flags & NM89_CLIP_LOOP) != 0U ? 1U : 0U;
    remaining = delta_ticks;
    while (remaining > 0U && rig->player.playing) {
        old_tick = rig->player.tick;
        rig->player.previous_tick = old_tick;
        if (!rig->player.reverse) {
            room = (nm89_u32)clip->length_ticks - old_tick;
            if (remaining <= room) {
                new_tick = (nm89_u16)(old_tick + remaining);
                nm89_emit_events_range(rig, clip, old_tick,
                                       new_tick, 0U);
                rig->player.tick = new_tick;
                remaining = 0U;
                if (rig->player.tick == clip->length_ticks) {
                    if (loop) {
                        rig->player.tick = 0U;
                    } else {
                        nm89_finish_or_queue(rig);
                    }
                }
            } else {
                nm89_emit_events_range(rig, clip, old_tick,
                                       clip->length_ticks, 0U);
                remaining -= room;
                if (loop) {
                    rig->player.tick = 0U;
                    if (room == 0U && remaining > 0U) {
                        remaining -= 1U;
                    }
                } else {
                    rig->player.tick = clip->length_ticks;
                    nm89_finish_or_queue(rig);
                }
            }
        } else {
            room = old_tick;
            if (remaining <= room) {
                new_tick = (nm89_u16)(old_tick - remaining);
                nm89_emit_events_range(rig, clip, old_tick,
                                       new_tick, 1U);
                rig->player.tick = new_tick;
                remaining = 0U;
                if (rig->player.tick == 0U) {
                    if (loop) {
                        rig->player.tick = clip->length_ticks;
                    } else {
                        nm89_finish_or_queue(rig);
                    }
                }
            } else {
                nm89_emit_events_range(rig, clip, old_tick, 0U, 1U);
                remaining -= room;
                if (loop) {
                    rig->player.tick = clip->length_ticks;
                    if (room == 0U && remaining > 0U) {
                        remaining -= 1U;
                    }
                } else {
                    rig->player.tick = 0U;
                    nm89_finish_or_queue(rig);
                }
            }
        }
        if (rig->player.playing &&
            rig->player.clip_id != NM89_INVALID_ID) {
            clip = &rig->clips[rig->player.clip_id];
            loop = (clip->flags & NM89_CLIP_LOOP) != 0U ? 1U : 0U;
        }
    }
}

int nm89_update(nm89_rig *rig, nm89_u16 delta_ticks)
{
    int result;
    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    nm89_advance_player(rig, delta_ticks);
    result = nm89_evaluate(rig);
    if (result != NM89_OK) {
        return result;
    }
    return nm89_update_world(rig);
}

int nm89_update_from_clock(nm89_rig *rig)
{
    nm89_provider *provider;
    nm89_u16 delta_ticks;
    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    provider = nm89_get_provider(rig, rig->clock_provider);
    if (provider == 0 || provider->sample_delta_ticks == 0) {
        return NM89_ERR_PROVIDER;
    }
    delta_ticks = provider->sample_delta_ticks(provider->user, rig);
    return nm89_update(rig, delta_ticks);
}

static int nm89_resolve_world_part(nm89_rig *rig, nm89_i16 part_id,
                                   nm89_u8 *state)
{
    nm89_part *part;
    nm89_vec3 pivot;
    nm89_provider *provider;
    nm89_transform_sample socket_sample;
    nm89_matrix local_matrix;
    nm89_matrix socket_matrix;
    nm89_matrix temporary;
    nm89_matrix external_world;
    int result;

    if (state[part_id] == 2U) {
        return NM89_OK;
    }
    if (state[part_id] == 1U) {
        return NM89_ERR_HIERARCHY;
    }
    state[part_id] = 1U;
    part = &rig->parts[part_id];

    if (part->parent_part != NM89_ROOT_PART) {
        result = nm89_resolve_world_part(rig, part->parent_part, state);
        if (result != NM89_OK) {
            return result;
        }
    }

    nm89_vec3_zero(&pivot);
    if (nm89_valid_pivot(rig, part->pivot_id)) {
        pivot = rig->pivots[part->pivot_id].point;
    }
    provider = nm89_get_provider(rig, part->pivot_provider);
    if (provider != 0 && provider->sample_pivot != 0) {
        result = provider->sample_pivot(provider->user, rig, part_id,
                                        part->pivot_source, &pivot);
        if (result != NM89_OK) {
            nm89_log_all(rig, NM89_LOG_WARNING, result,
                         "pivot provider sample failed");
        }
    }

    nm89_math_from_transform(rig, &local_matrix,
                             &part->pose.resolved, &pivot);
    part->world_visible = part->pose.resolved_visible;

    provider = nm89_get_provider(rig, part->socket_provider);
    if (provider != 0 && provider->sample_socket != 0) {
        nm89_transform_sample_identity(&socket_sample);
        result = provider->sample_socket(provider->user, rig, part_id,
                                         part->socket_source,
                                         &socket_sample);
        if (result == NM89_OK) {
            nm89_math_from_transform(rig, &socket_matrix,
                                     &socket_sample.transform, 0);
            if (socket_sample.has_visibility) {
                part->world_visible =
                    (part->world_visible &&
                     socket_sample.visible) ? 1U : 0U;
            }
            if (part->socket_space == NM89_SOCKET_WORLD_SPACE) {
                nm89_math_multiply(rig, &part->world,
                                   &socket_matrix, &local_matrix);
            } else {
                nm89_math_multiply(rig, &temporary,
                                   &socket_matrix, &local_matrix);
                if (part->parent_part != NM89_ROOT_PART) {
                    nm89_math_multiply(
                        rig, &part->world,
                        &rig->parts[part->parent_part].world,
                        &temporary);
                } else {
                    part->world = temporary;
                }
            }
        } else {
            nm89_log_all(rig, NM89_LOG_WARNING, result,
                         "socket provider sample failed");
            if (part->parent_part != NM89_ROOT_PART) {
                nm89_math_multiply(
                    rig, &part->world,
                    &rig->parts[part->parent_part].world,
                    &local_matrix);
            } else {
                part->world = local_matrix;
            }
        }
    } else {
        if (part->parent_part != NM89_ROOT_PART) {
            nm89_math_multiply(rig, &part->world,
                               &rig->parts[part->parent_part].world,
                               &local_matrix);
        } else {
            part->world = local_matrix;
        }
    }

    provider = nm89_get_provider(rig, part->world_provider);
    if (provider != 0 && provider->sample_world != 0) {
        nm89_math_identity(rig, &external_world);
        result = provider->sample_world(provider->user, rig, part_id,
                                        part->world_source,
                                        &external_world);
        if (result == NM89_OK) {
            if (part->world_mode == NM89_WORLD_REPLACE) {
                part->world = external_world;
            } else {
                nm89_math_multiply(rig, &temporary,
                                   &external_world, &part->world);
                part->world = temporary;
            }
        } else {
            nm89_log_all(rig, NM89_LOG_WARNING, result,
                         "world provider sample failed");
        }
    }

    if (part->parent_part != NM89_ROOT_PART) {
        part->world_visible =
            (part->world_visible &&
             rig->parts[part->parent_part].world_visible) ?
            1U : 0U;
    }
    state[part_id] = 2U;
    return NM89_OK;
}

int nm89_update_world(nm89_rig *rig)
{
    nm89_u8 state[NM89_MAX_PARTS];
    nm89_u16 i;
    int result;
    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    for (i = 0U; i < rig->part_count; ++i) {
        state[i] = 0U;
    }
    for (i = 0U; i < rig->part_count; ++i) {
        result = nm89_resolve_world_part(rig, (nm89_i16)i, state);
        if (result != NM89_OK) {
            nm89_log_all(rig, NM89_LOG_ERROR, result,
                         "hierarchy resolution failed");
            return result;
        }
    }
    return NM89_OK;
}

int nm89_flush(nm89_rig *rig)
{
    nm89_u16 i;
    nm89_u16 p;
    nm89_binding *binding;
    nm89_part *part;
    nm89_geometry_resolution resolution;
    nm89_geometry_packet packet;
    nm89_provider *provider;
    nm89_matrix alignment_matrix;
    nm89_matrix final_world;
    nm89_u8 provider_visible;
    int result;
    if (rig == 0) {
        return NM89_ERR_ARGUMENT;
    }
    for (i = 0U; i < rig->binding_count; ++i) {
        binding = &rig->bindings[i];
        if (!nm89_valid_part(rig, binding->owner_part)) {
            continue;
        }
        part = &rig->parts[binding->owner_part];
        resolution.resource_id = binding->resource_id;
        resolution.selector_id = binding->selector_id;
        resolution.mesh_id = binding->mesh_id;
        resolution.alignment_id = binding->alignment_id;
        resolution.selector_type = binding->selector_type;
        resolution.visible = 1U;

        provider = nm89_get_provider(rig,
                                     binding->resolver_provider);
        if (provider != 0 && provider->resolve_geometry != 0) {
            result = provider->resolve_geometry(
                provider->user, rig, (nm89_i16)i,
                binding, &resolution);
            if (result != NM89_OK) {
                nm89_log_all(rig, NM89_LOG_WARNING, result,
                             "geometry resolver failed");
            }
        }

        final_world = part->world;
        if (nm89_valid_alignment(rig, resolution.alignment_id)) {
            nm89_math_from_transform(
                rig, &alignment_matrix,
                &rig->alignments[resolution.alignment_id].transform,
                0);
            nm89_math_multiply(rig, &final_world,
                               &part->world, &alignment_matrix);
        }

        packet.binding_id = (nm89_i16)i;
        packet.part_id = binding->owner_part;
        packet.parent_part = part->parent_part;
        packet.part_tag = part->user_tag;
        packet.binding_tag = binding->user_tag;
        packet.resource_id = resolution.resource_id;
        packet.selector_id = resolution.selector_id;
        packet.mesh_id = resolution.mesh_id;
        packet.selector_type = resolution.selector_type;
        packet.local_resolved = part->pose.resolved;
        packet.world = final_world;
        packet.visible =
            ((binding->flags & NM89_BINDING_VISIBLE) != 0U &&
             part->world_visible && resolution.visible) ? 1U : 0U;

        provider = nm89_get_provider(rig,
                                     part->visibility_provider);
        if (provider != 0 && provider->sample_visibility != 0) {
            provider_visible = 1U;
            result = provider->sample_visibility(
                provider->user, rig, binding->owner_part,
                (nm89_i16)i, part->visibility_source,
                &provider_visible);
            if (result == NM89_OK) {
                if (part->visibility_mode ==
                    NM89_VISIBILITY_REPLACE) {
                    packet.visible = provider_visible ? 1U : 0U;
                } else {
                    packet.visible =
                        (packet.visible && provider_visible) ? 1U : 0U;
                }
            }
        }

        for (p = 0U; p < rig->provider_count; ++p) {
            if (rig->providers[p].apply_geometry != 0) {
                rig->providers[p].apply_geometry(
                    rig->providers[p].user, rig, &packet);
            }
        }
    }
    return NM89_OK;
}

int nm89_step(nm89_rig *rig, nm89_u16 delta_ticks,
              nm89_u8 flush_output)
{
    int result;
    result = nm89_update(rig, delta_ticks);
    if (result != NM89_OK) {
        return result;
    }
    if (flush_output) {
        return nm89_flush(rig);
    }
    return NM89_OK;
}

const nm89_pose *nm89_part_get_pose(const nm89_rig *rig,
                                    nm89_i16 part_id)
{
    if (!nm89_valid_part(rig, part_id)) {
        return 0;
    }
    return &rig->parts[part_id].pose;
}

const nm89_matrix *nm89_part_get_world(const nm89_rig *rig,
                                       nm89_i16 part_id)
{
    if (!nm89_valid_part(rig, part_id)) {
        return 0;
    }
    return &rig->parts[part_id].world;
}
