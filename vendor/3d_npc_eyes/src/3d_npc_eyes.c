#include "3d_npc_eyes.h"

static const tdne_i32 tdne_cos_table_181[181] = {
    1024L, 1024L, 1023L, 1023L, 1022L, 1020L, 1018L, 1016L, 1014L, 1011L, 1008L, 1005L,
    1002L, 998L, 994L, 989L, 984L, 979L, 974L, 968L, 962L, 956L, 949L, 943L,
    935L, 928L, 920L, 912L, 904L, 896L, 887L, 878L, 868L, 859L, 849L, 839L,
    828L, 818L, 807L, 796L, 784L, 773L, 761L, 749L, 737L, 724L, 711L, 698L,
    685L, 672L, 658L, 644L, 630L, 616L, 602L, 587L, 573L, 558L, 543L, 527L,
    512L, 496L, 481L, 465L, 449L, 433L, 416L, 400L, 384L, 367L, 350L, 333L,
    316L, 299L, 282L, 265L, 248L, 230L, 213L, 195L, 178L, 160L, 143L, 125L,
    107L, 89L, 71L, 54L, 36L, 18L, 0L, -18L, -36L, -54L, -71L, -89L,
    -107L, -125L, -143L, -160L, -178L, -195L, -213L, -230L, -248L, -265L, -282L, -299L,
    -316L, -333L, -350L, -367L, -384L, -400L, -416L, -433L, -449L, -465L, -481L, -496L,
    -512L, -527L, -543L, -558L, -573L, -587L, -602L, -616L, -630L, -644L, -658L, -672L,
    -685L, -698L, -711L, -724L, -737L, -749L, -761L, -773L, -784L, -796L, -807L, -818L,
    -828L, -839L, -849L, -859L, -868L, -878L, -887L, -896L, -904L, -912L, -920L, -928L,
    -935L, -943L, -949L, -956L, -962L, -968L, -974L, -979L, -984L, -989L, -994L, -998L,
    -1002L, -1005L, -1008L, -1011L, -1014L, -1016L, -1018L, -1020L, -1022L, -1023L, -1023L, -1024L,
    -1024L
};

static const tdne_i32 tdne_tan_table_90[90] = {
    0L, 18L, 36L, 54L, 72L, 90L, 108L, 126L, 144L, 162L, 181L, 199L,
    218L, 236L, 255L, 274L, 294L, 313L, 333L, 353L, 373L, 393L, 414L, 435L,
    456L, 477L, 499L, 522L, 544L, 568L, 591L, 615L, 640L, 665L, 691L, 717L,
    744L, 772L, 800L, 829L, 859L, 890L, 922L, 955L, 989L, 1024L, 1060L, 1098L,
    1137L, 1178L, 1220L, 1265L, 1311L, 1359L, 1409L, 1462L, 1518L, 1577L, 1639L, 1704L,
    1774L, 1847L, 1926L, 2010L, 2100L, 2196L, 2300L, 2412L, 2534L, 2668L, 2813L, 2974L,
    3152L, 3349L, 3571L, 3822L, 4107L, 4435L, 4818L, 5268L, 5807L, 6465L, 7286L, 8340L,
    9743L, 11704L, 14644L, 19539L, 29324L, 65535L
};

static tdne_i32 tdne_clamp_i32(tdne_i32 v, tdne_i32 lo, tdne_i32 hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static int tdne_same_sign(tdne_i32 a, tdne_i32 b)
{
    if (a < 0 && b < 0) {
        return TDNE_TRUE;
    }
    if (a >= 0 && b >= 0) {
        return TDNE_TRUE;
    }
    return TDNE_FALSE;
}

static tdne_i32 tdne_mul_div_nonnegative_sat(tdne_i32 value, tdne_i32 factor, tdne_i32 divisor)
{
    tdne_i32 q;
    tdne_i32 r;
    tdne_i32 a;
    tdne_i32 b;
    tdne_i32 tail;
    tdne_i32 out;

    if (value <= 0 || factor <= 0 || divisor <= 0) {
        return 0;
    }

    q = value / divisor;
    r = value % divisor;

    if (q != 0 && factor > LONG_MAX / q) {
        return LONG_MAX;
    }

    out = q * factor;

    if (r != 0 && factor > LONG_MAX / r) {
        tail = LONG_MAX;
    } else {
        tail = (r * factor) / divisor;
    }

    if (LONG_MAX - out < tail) {
        return LONG_MAX;
    }

    a = out + tail;
    b = LONG_MAX;
    if (a > b) {
        return b;
    }
    return a;
}

static tdne_i32 tdne_mul_div_signed_sat(tdne_i32 value, tdne_i32 factor, tdne_i32 divisor)
{
    int sign;
    tdne_i32 a;
    tdne_i32 b;
    tdne_i32 out;

    if (divisor <= 0) {
        return 0;
    }

    sign = TDNE_FALSE;
    if (value < 0) {
        sign = !sign;
    }
    if (factor < 0) {
        sign = !sign;
    }

    a = tdne_abs_i32(value);
    b = tdne_abs_i32(factor);
    out = tdne_mul_div_nonnegative_sat(a, b, divisor);

    if (sign) {
        if (out >= LONG_MAX) {
            return LONG_MIN + 1;
        }
        return -out;
    }

    return out;
}

static tdne_i32 tdne_ratio_bounded_to_scale(tdne_i32 value_abs, tdne_i32 divisor, tdne_i32 scale)
{
    tdne_i32 out;
    tdne_i32 rem;
    tdne_i32 i;
    tdne_i32 step;

    if (value_abs <= 0 || divisor <= 0 || scale <= 0) {
        return 0;
    }

    if (value_abs > divisor) {
        value_abs = divisor;
    }

    out = 0;
    rem = 0;
    step = divisor - value_abs;

    for (i = 0; i < scale; ++i) {
        if (value_abs == divisor) {
            ++out;
        } else if (rem >= step) {
            ++out;
            rem = rem - step;
        } else {
            rem = rem + value_abs;
        }
    }

    return out;
}

static tdne_i32 tdne_normalized_component(tdne_i32 value, tdne_i32 length)
{
    tdne_i32 a;
    tdne_i32 out;

    if (length <= 0 || value == 0) {
        return 0;
    }

    a = tdne_abs_i32(value);
    out = tdne_ratio_bounded_to_scale(a, length, TDNE_DIR_SCALE);

    if (value < 0) {
        return -out;
    }
    return out;
}

static tdne_i32 tdne_project_axis(tdne_vec3 delta, tdne_vec3 axis)
{
    tdne_i32 x;
    tdne_i32 y;
    tdne_i32 z;
    tdne_i32 xy;

    x = tdne_mul_div_signed_sat(delta.x, axis.x, TDNE_DIR_SCALE);
    y = tdne_mul_div_signed_sat(delta.y, axis.y, TDNE_DIR_SCALE);
    z = tdne_mul_div_signed_sat(delta.z, axis.z, TDNE_DIR_SCALE);
    xy = tdne_add_sat_i32(x, y);
    return tdne_add_sat_i32(xy, z);
}

static void tdne_result_clear(tdne_result *r)
{
    if (r == 0) {
        return;
    }

    r->visible = TDNE_FALSE;
    r->target_index = -1;
    r->target = 0;
    r->visibility = TDNE_VIS_NONE;
    r->reason = TDNE_VIS_NONE;
    r->score = 0;
    r->distance = 0;
    r->distance_sq = 0;
    r->dot = 0;
    r->samples_total = 0;
    r->samples_in_shape = 0;
    r->rays_clear = 0;
    r->rays_blocked = 0;
    r->last_seen_point = tdne_vec3_zero();
    r->hit.hit = TDNE_FALSE;
    r->hit.point = tdne_vec3_zero();
    r->hit.normal = tdne_vec3_zero();
    r->hit.material_mask = 0;
    r->hit.user = 0;
}

static tdne_vec3 tdne_sample_point(const tdne_target *target, int index)
{
    tdne_vec3 p;

    p = target->origin;
    if (target->sample_count > 0 && index >= 0 && index < target->sample_count) {
        p = tdne_vec3_add(p, target->samples[index]);
    }
    return p;
}

static int tdne_line_clear(
    const tdne_sensor *sensor,
    tdne_vec3 point,
    tdne_raycast_fn raycast,
    void *world_user,
    tdne_ray_hit *out_hit
)
{
    tdne_ray_hit local_hit;
    int blocked;

    if (out_hit != 0) {
        out_hit->hit = TDNE_FALSE;
        out_hit->point = tdne_vec3_zero();
        out_hit->normal = tdne_vec3_zero();
        out_hit->material_mask = 0;
        out_hit->user = 0;
    }

    if (sensor->require_line_of_sight == TDNE_FALSE || raycast == 0) {
        return TDNE_TRUE;
    }

    local_hit.hit = TDNE_FALSE;
    local_hit.point = tdne_vec3_zero();
    local_hit.normal = tdne_vec3_zero();
    local_hit.material_mask = 0;
    local_hit.user = 0;

    blocked = raycast(world_user, &sensor->origin, &point, sensor->block_mask, &local_hit);
    if (blocked) {
        local_hit.hit = TDNE_TRUE;
        if (out_hit != 0) {
            *out_hit = local_hit;
        }
        return TDNE_FALSE;
    }

    if (out_hit != 0) {
        *out_hit = local_hit;
    }
    return TDNE_TRUE;
}

static tdne_i32 tdne_distance_score(tdne_i32 distance, tdne_i32 max_distance)
{
    tdne_i32 d;
    tdne_i32 p;

    if (max_distance <= 0) {
        return TDNE_SCORE_SCALE;
    }

    d = distance;
    if (d < 0) {
        d = 0;
    }
    if (d > max_distance) {
        d = max_distance;
    }

    p = tdne_mul_div_nonnegative_sat(d, TDNE_SCORE_SCALE, max_distance);
    if (p >= TDNE_SCORE_SCALE) {
        return 0;
    }
    return TDNE_SCORE_SCALE - p;
}

static void tdne_sort_results_by_score(tdne_result *results, int count)
{
    int i;
    int j;
    int best;
    tdne_result tmp;

    if (results == 0 || count <= 1) {
        return;
    }

    for (i = 0; i < count - 1; ++i) {
        best = i;
        for (j = i + 1; j < count; ++j) {
            if (results[j].score > results[best].score) {
                best = j;
            } else if (results[j].score == results[best].score &&
                       results[j].distance < results[best].distance) {
                best = j;
            }
        }
        if (best != i) {
            tmp = results[i];
            results[i] = results[best];
            results[best] = tmp;
        }
    }
}

tdne_vec3 tdne_vec3_make(tdne_i32 x, tdne_i32 y, tdne_i32 z)
{
    tdne_vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

tdne_vec3 tdne_vec3_zero(void)
{
    return tdne_vec3_make(0, 0, 0);
}

tdne_vec3 tdne_vec3_add(tdne_vec3 a, tdne_vec3 b)
{
    return tdne_vec3_make(
        tdne_add_sat_i32(a.x, b.x),
        tdne_add_sat_i32(a.y, b.y),
        tdne_add_sat_i32(a.z, b.z)
    );
}

tdne_vec3 tdne_vec3_sub(tdne_vec3 a, tdne_vec3 b)
{
    return tdne_vec3_make(
        tdne_sub_sat_i32(a.x, b.x),
        tdne_sub_sat_i32(a.y, b.y),
        tdne_sub_sat_i32(a.z, b.z)
    );
}

tdne_vec3 tdne_vec3_neg(tdne_vec3 a)
{
    return tdne_vec3_make(
        tdne_sub_sat_i32(0, a.x),
        tdne_sub_sat_i32(0, a.y),
        tdne_sub_sat_i32(0, a.z)
    );
}

tdne_i32 tdne_abs_i32(tdne_i32 v)
{
    if (v == LONG_MIN) {
        return LONG_MAX;
    }
    if (v < 0) {
        return -v;
    }
    return v;
}

tdne_i32 tdne_add_sat_i32(tdne_i32 a, tdne_i32 b)
{
    if (b > 0 && a > LONG_MAX - b) {
        return LONG_MAX;
    }
    if (b < 0 && a < LONG_MIN - b) {
        return LONG_MIN;
    }
    return a + b;
}

tdne_i32 tdne_sub_sat_i32(tdne_i32 a, tdne_i32 b)
{
    if (b == LONG_MIN) {
        if (a >= 0) {
            return LONG_MAX;
        }
        return tdne_add_sat_i32(a, LONG_MAX);
    }
    return tdne_add_sat_i32(a, -b);
}

tdne_i32 tdne_mul_sat_i32(tdne_i32 a, tdne_i32 b)
{
    int negative;
    tdne_i32 aa;
    tdne_i32 bb;
    tdne_i32 out;

    if (a == 0 || b == 0) {
        return 0;
    }

    negative = !tdne_same_sign(a, b);
    aa = tdne_abs_i32(a);
    bb = tdne_abs_i32(b);

    if (aa != 0 && bb > LONG_MAX / aa) {
        if (negative) {
            return LONG_MIN + 1;
        }
        return LONG_MAX;
    }

    out = aa * bb;
    if (negative) {
        return -out;
    }
    return out;
}

tdne_i32 tdne_vec3_len_sq_sat(tdne_vec3 v)
{
    tdne_i32 x;
    tdne_i32 y;
    tdne_i32 z;
    tdne_i32 xy;

    x = tdne_mul_sat_i32(v.x, v.x);
    y = tdne_mul_sat_i32(v.y, v.y);
    z = tdne_mul_sat_i32(v.z, v.z);
    xy = tdne_add_sat_i32(x, y);
    return tdne_add_sat_i32(xy, z);
}

tdne_i32 tdne_vec3_distance_sq_sat(tdne_vec3 a, tdne_vec3 b)
{
    return tdne_vec3_len_sq_sat(tdne_vec3_sub(a, b));
}

tdne_i32 tdne_i32_sqrt(tdne_i32 v)
{
    tdne_i32 lo;
    tdne_i32 hi;
    tdne_i32 mid;
    tdne_i32 ans;

    if (v <= 0) {
        return 0;
    }

    lo = 1;
    hi = 1;
    while (hi <= v / hi && hi <= LONG_MAX / 2) {
        hi = hi * 2;
    }
    hi = hi - 1;
    ans = 0;

    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;
        if (mid <= v / mid) {
            ans = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    return ans;
}

int tdne_vec3_normalize_dir(tdne_vec3 v, tdne_vec3 *out_dir)
{
    tdne_i32 len_sq;
    tdne_i32 len;

    if (out_dir == 0) {
        return TDNE_FALSE;
    }

    len_sq = tdne_vec3_len_sq_sat(v);
    len = tdne_i32_sqrt(len_sq);

    if (len <= 0) {
        *out_dir = tdne_vec3_zero();
        return TDNE_FALSE;
    }

    out_dir->x = tdne_normalized_component(v.x, len);
    out_dir->y = tdne_normalized_component(v.y, len);
    out_dir->z = tdne_normalized_component(v.z, len);
    return TDNE_TRUE;
}

tdne_i32 tdne_dir_dot(tdne_vec3 a, tdne_vec3 b)
{
    tdne_i32 x;
    tdne_i32 y;
    tdne_i32 z;
    tdne_i32 xy;

    x = tdne_mul_div_signed_sat(a.x, b.x, TDNE_DIR_SCALE);
    y = tdne_mul_div_signed_sat(a.y, b.y, TDNE_DIR_SCALE);
    z = tdne_mul_div_signed_sat(a.z, b.z, TDNE_DIR_SCALE);
    xy = tdne_add_sat_i32(x, y);
    return tdne_add_sat_i32(xy, z);
}

tdne_i32 tdne_cos_deg(int degrees)
{
    if (degrees < 0) {
        degrees = 0;
    }
    if (degrees > 180) {
        degrees = 180;
    }
    return tdne_cos_table_181[degrees];
}

tdne_i32 tdne_tan_deg(int degrees)
{
    if (degrees < 0) {
        degrees = 0;
    }
    if (degrees > 89) {
        degrees = 89;
    }
    return tdne_tan_table_90[degrees];
}

void tdne_sensor_init_defaults(tdne_sensor *sensor)
{
    if (sensor == 0) {
        return;
    }

    sensor->shape = TDNE_SENSOR_CONE;
    sensor->origin = tdne_vec3_zero();
    sensor->forward = tdne_vec3_make(0, 0, TDNE_DIR_SCALE);
    sensor->right = tdne_vec3_make(TDNE_DIR_SCALE, 0, 0);
    sensor->up = tdne_vec3_make(0, TDNE_DIR_SCALE, 0);
    sensor->max_distance = 1000;
    sensor->near_distance = 0;
    sensor->cos_half_fov = tdne_cos_deg(45);
    sensor->tan_half_h = tdne_tan_deg(45);
    sensor->tan_half_v = tdne_tan_deg(45);
    sensor->box_half = tdne_vec3_make(100, 100, 100);
    sensor->see_mask = TDNE_MASK_ALL;
    sensor->block_mask = TDNE_MASK_ALL;
    sensor->require_line_of_sight = TDNE_TRUE;
}

void tdne_sensor_init_cone(
    tdne_sensor *sensor,
    tdne_vec3 origin,
    tdne_vec3 forward,
    tdne_i32 max_distance,
    int fov_degrees
)
{
    tdne_sensor_init_defaults(sensor);
    if (sensor == 0) {
        return;
    }

    sensor->shape = TDNE_SENSOR_CONE;
    sensor->origin = origin;
    sensor->max_distance = max_distance;
    tdne_sensor_set_forward(sensor, forward);
    tdne_sensor_set_cone_degrees(sensor, fov_degrees);
}

void tdne_sensor_init_sphere(
    tdne_sensor *sensor,
    tdne_vec3 origin,
    tdne_i32 max_distance
)
{
    tdne_sensor_init_defaults(sensor);
    if (sensor == 0) {
        return;
    }

    sensor->shape = TDNE_SENSOR_SPHERE;
    sensor->origin = origin;
    sensor->max_distance = max_distance;
}

void tdne_sensor_init_box(
    tdne_sensor *sensor,
    tdne_vec3 origin,
    tdne_vec3 half_extents
)
{
    tdne_sensor_init_defaults(sensor);
    if (sensor == 0) {
        return;
    }

    sensor->shape = TDNE_SENSOR_BOX;
    sensor->origin = origin;
    sensor->box_half.x = tdne_abs_i32(half_extents.x);
    sensor->box_half.y = tdne_abs_i32(half_extents.y);
    sensor->box_half.z = tdne_abs_i32(half_extents.z);
    sensor->max_distance = tdne_i32_sqrt(tdne_vec3_len_sq_sat(sensor->box_half));
}

void tdne_sensor_init_frustum(
    tdne_sensor *sensor,
    tdne_vec3 origin,
    tdne_vec3 forward,
    tdne_vec3 right,
    tdne_vec3 up,
    tdne_i32 near_distance,
    tdne_i32 max_distance,
    int horizontal_fov_degrees,
    int vertical_fov_degrees
)
{
    tdne_sensor_init_defaults(sensor);
    if (sensor == 0) {
        return;
    }

    sensor->shape = TDNE_SENSOR_FRUSTUM;
    sensor->origin = origin;
    sensor->near_distance = near_distance;
    sensor->max_distance = max_distance;
    tdne_sensor_set_axes(sensor, forward, right, up);
    tdne_sensor_set_frustum_degrees(sensor, horizontal_fov_degrees, vertical_fov_degrees);
}

void tdne_sensor_set_origin(tdne_sensor *sensor, tdne_vec3 origin)
{
    if (sensor != 0) {
        sensor->origin = origin;
    }
}

int tdne_sensor_set_forward(tdne_sensor *sensor, tdne_vec3 forward)
{
    tdne_vec3 out;

    if (sensor == 0) {
        return TDNE_FALSE;
    }
    if (!tdne_vec3_normalize_dir(forward, &out)) {
        return TDNE_FALSE;
    }
    sensor->forward = out;
    return TDNE_TRUE;
}

int tdne_sensor_set_axes(
    tdne_sensor *sensor,
    tdne_vec3 forward,
    tdne_vec3 right,
    tdne_vec3 up
)
{
    tdne_vec3 f;
    tdne_vec3 r;
    tdne_vec3 u;

    if (sensor == 0) {
        return TDNE_FALSE;
    }
    if (!tdne_vec3_normalize_dir(forward, &f)) {
        return TDNE_FALSE;
    }
    if (!tdne_vec3_normalize_dir(right, &r)) {
        return TDNE_FALSE;
    }
    if (!tdne_vec3_normalize_dir(up, &u)) {
        return TDNE_FALSE;
    }

    sensor->forward = f;
    sensor->right = r;
    sensor->up = u;
    return TDNE_TRUE;
}

void tdne_sensor_set_masks(tdne_sensor *sensor, tdne_u32 see_mask, tdne_u32 block_mask)
{
    if (sensor == 0) {
        return;
    }
    sensor->see_mask = see_mask;
    sensor->block_mask = block_mask;
}

void tdne_sensor_set_line_of_sight(tdne_sensor *sensor, int required)
{
    if (sensor == 0) {
        return;
    }
    sensor->require_line_of_sight = required ? TDNE_TRUE : TDNE_FALSE;
}

void tdne_sensor_set_cone_degrees(tdne_sensor *sensor, int fov_degrees)
{
    int half;

    if (sensor == 0) {
        return;
    }

    if (fov_degrees < 1) {
        fov_degrees = 1;
    }
    if (fov_degrees > 180) {
        fov_degrees = 180;
    }

    half = (fov_degrees + 1) / 2;
    sensor->cos_half_fov = tdne_cos_deg(half);
}

void tdne_sensor_set_frustum_degrees(
    tdne_sensor *sensor,
    int horizontal_fov_degrees,
    int vertical_fov_degrees
)
{
    int hh;
    int hv;

    if (sensor == 0) {
        return;
    }

    horizontal_fov_degrees = (int)tdne_clamp_i32((tdne_i32)horizontal_fov_degrees, 1, 178);
    vertical_fov_degrees = (int)tdne_clamp_i32((tdne_i32)vertical_fov_degrees, 1, 178);

    hh = (horizontal_fov_degrees + 1) / 2;
    hv = (vertical_fov_degrees + 1) / 2;

    sensor->tan_half_h = tdne_tan_deg(hh);
    sensor->tan_half_v = tdne_tan_deg(hv);
}

void tdne_target_init(
    tdne_target *target,
    tdne_vec3 origin,
    tdne_i32 radius,
    tdne_u32 mask,
    void *user
)
{
    if (target == 0) {
        return;
    }

    target->origin = origin;
    target->radius = tdne_abs_i32(radius);
    target->mask = mask;
    target->user = user;
    target->sample_count = 0;
    tdne_target_use_center(target);
}

void tdne_target_clear_samples(tdne_target *target)
{
    int i;

    if (target == 0) {
        return;
    }

    target->sample_count = 0;
    for (i = 0; i < TDNE_MAX_TARGET_SAMPLES; ++i) {
        target->samples[i] = tdne_vec3_zero();
    }
}

int tdne_target_add_sample(tdne_target *target, tdne_vec3 local_offset)
{
    if (target == 0) {
        return TDNE_FALSE;
    }
    if (target->sample_count >= TDNE_MAX_TARGET_SAMPLES) {
        return TDNE_FALSE;
    }

    target->samples[target->sample_count] = local_offset;
    target->sample_count += 1;
    return TDNE_TRUE;
}

int tdne_target_use_center(tdne_target *target)
{
    if (target == 0) {
        return TDNE_FALSE;
    }
    tdne_target_clear_samples(target);
    return tdne_target_add_sample(target, tdne_vec3_zero());
}

int tdne_target_use_vertical3(tdne_target *target, tdne_i32 half_height, int axis)
{
    tdne_vec3 a;
    tdne_vec3 b;

    if (target == 0) {
        return TDNE_FALSE;
    }

    half_height = tdne_abs_i32(half_height);
    a = tdne_vec3_zero();
    b = tdne_vec3_zero();

    if (axis == TDNE_AXIS_X) {
        a.x = half_height;
        b.x = -half_height;
    } else if (axis == TDNE_AXIS_Z) {
        a.z = half_height;
        b.z = -half_height;
    } else {
        a.y = half_height;
        b.y = -half_height;
    }

    tdne_target_clear_samples(target);
    if (!tdne_target_add_sample(target, tdne_vec3_zero())) {
        return TDNE_FALSE;
    }
    if (!tdne_target_add_sample(target, a)) {
        return TDNE_FALSE;
    }
    if (!tdne_target_add_sample(target, b)) {
        return TDNE_FALSE;
    }
    return TDNE_TRUE;
}

int tdne_point_test_sensor(
    const tdne_sensor *sensor,
    tdne_vec3 point,
    tdne_result *out_probe
)
{
    tdne_vec3 delta;
    tdne_vec3 dir;
    tdne_i32 dist_sq;
    tdne_i32 dist;
    tdne_i32 dot;
    tdne_i32 x;
    tdne_i32 y;
    tdne_i32 z;
    tdne_i32 hx;
    tdne_i32 hy;

    if (out_probe != 0) {
        tdne_result_clear(out_probe);
    }

    if (sensor == 0) {
        if (out_probe != 0) {
            out_probe->reason = TDNE_VIS_BAD_INPUT;
            out_probe->visibility = TDNE_VIS_BAD_INPUT;
        }
        return TDNE_VIS_BAD_INPUT;
    }

    delta = tdne_vec3_sub(point, sensor->origin);
    dist_sq = tdne_vec3_len_sq_sat(delta);
    dist = tdne_i32_sqrt(dist_sq);
    dot = 0;

    if (out_probe != 0) {
        out_probe->distance_sq = dist_sq;
        out_probe->distance = dist;
    }

    if (sensor->shape == TDNE_SENSOR_BOX) {
        if (tdne_abs_i32(delta.x) > sensor->box_half.x ||
            tdne_abs_i32(delta.y) > sensor->box_half.y ||
            tdne_abs_i32(delta.z) > sensor->box_half.z) {
            if (out_probe != 0) {
                out_probe->reason = TDNE_VIS_OUT_OF_SHAPE;
                out_probe->visibility = TDNE_VIS_OUT_OF_SHAPE;
            }
            return TDNE_VIS_OUT_OF_SHAPE;
        }
        if (out_probe != 0) {
            out_probe->reason = TDNE_VIS_VISIBLE;
            out_probe->visibility = TDNE_VIS_VISIBLE;
        }
        return TDNE_VIS_VISIBLE;
    }

    if (sensor->max_distance >= 0 && dist > sensor->max_distance) {
        if (out_probe != 0) {
            out_probe->reason = TDNE_VIS_OUT_OF_RANGE;
            out_probe->visibility = TDNE_VIS_OUT_OF_RANGE;
        }
        return TDNE_VIS_OUT_OF_RANGE;
    }

    if (sensor->shape == TDNE_SENSOR_SPHERE) {
        if (out_probe != 0) {
            out_probe->reason = TDNE_VIS_VISIBLE;
            out_probe->visibility = TDNE_VIS_VISIBLE;
        }
        return TDNE_VIS_VISIBLE;
    }

    if (sensor->shape == TDNE_SENSOR_FRUSTUM) {
        z = tdne_project_axis(delta, sensor->forward);
        if (z < sensor->near_distance || z > sensor->max_distance) {
            if (out_probe != 0) {
                out_probe->dot = z;
                out_probe->reason = TDNE_VIS_OUT_OF_RANGE;
                out_probe->visibility = TDNE_VIS_OUT_OF_RANGE;
            }
            return TDNE_VIS_OUT_OF_RANGE;
        }

        x = tdne_project_axis(delta, sensor->right);
        y = tdne_project_axis(delta, sensor->up);
        hx = tdne_mul_div_nonnegative_sat(z, sensor->tan_half_h, TDNE_DIR_SCALE);
        hy = tdne_mul_div_nonnegative_sat(z, sensor->tan_half_v, TDNE_DIR_SCALE);

        if (out_probe != 0) {
            out_probe->dot = z;
        }

        if (tdne_abs_i32(x) > hx || tdne_abs_i32(y) > hy) {
            if (out_probe != 0) {
                out_probe->reason = TDNE_VIS_OUT_OF_SHAPE;
                out_probe->visibility = TDNE_VIS_OUT_OF_SHAPE;
            }
            return TDNE_VIS_OUT_OF_SHAPE;
        }

        if (out_probe != 0) {
            out_probe->reason = TDNE_VIS_VISIBLE;
            out_probe->visibility = TDNE_VIS_VISIBLE;
        }
        return TDNE_VIS_VISIBLE;
    }

    if (dist <= 0) {
        if (out_probe != 0) {
            out_probe->dot = TDNE_DIR_SCALE;
            out_probe->reason = TDNE_VIS_VISIBLE;
            out_probe->visibility = TDNE_VIS_VISIBLE;
        }
        return TDNE_VIS_VISIBLE;
    }

    if (!tdne_vec3_normalize_dir(delta, &dir)) {
        if (out_probe != 0) {
            out_probe->reason = TDNE_VIS_BAD_INPUT;
            out_probe->visibility = TDNE_VIS_BAD_INPUT;
        }
        return TDNE_VIS_BAD_INPUT;
    }

    dot = tdne_dir_dot(sensor->forward, dir);
    if (out_probe != 0) {
        out_probe->dot = dot;
    }

    if (dot < sensor->cos_half_fov) {
        if (out_probe != 0) {
            out_probe->reason = TDNE_VIS_OUT_OF_SHAPE;
            out_probe->visibility = TDNE_VIS_OUT_OF_SHAPE;
        }
        return TDNE_VIS_OUT_OF_SHAPE;
    }

    if (out_probe != 0) {
        out_probe->reason = TDNE_VIS_VISIBLE;
        out_probe->visibility = TDNE_VIS_VISIBLE;
    }
    return TDNE_VIS_VISIBLE;
}

int tdne_eval_target(
    const tdne_sensor *sensor,
    const tdne_target *target,
    tdne_raycast_fn raycast,
    void *world_user,
    tdne_result *out_result
)
{
    tdne_result result;
    tdne_result probe;
    tdne_ray_hit hit;
    tdne_vec3 p;
    int sample_total;
    int i;
    int state;
    int first_reject;
    int clear;
    tdne_i32 sample_score;
    tdne_i32 dist_score;

    tdne_result_clear(&result);

    if (sensor == 0 || target == 0) {
        result.visibility = TDNE_VIS_BAD_INPUT;
        result.reason = TDNE_VIS_BAD_INPUT;
        if (out_result != 0) {
            *out_result = result;
        }
        return TDNE_VIS_BAD_INPUT;
    }

    result.target = target;

    if ((sensor->see_mask & target->mask) == 0UL) {
        result.visibility = TDNE_VIS_MASKED;
        result.reason = TDNE_VIS_MASKED;
        if (out_result != 0) {
            *out_result = result;
        }
        return TDNE_VIS_MASKED;
    }

    sample_total = target->sample_count;
    if (sample_total <= 0) {
        sample_total = 1;
    }
    if (sample_total > TDNE_MAX_TARGET_SAMPLES) {
        sample_total = TDNE_MAX_TARGET_SAMPLES;
    }

    result.samples_total = sample_total;
    first_reject = TDNE_VIS_NONE;

    for (i = 0; i < sample_total; ++i) {
        p = tdne_sample_point(target, i);
        state = tdne_point_test_sensor(sensor, p, &probe);

        if (i == 0) {
            result.distance = probe.distance;
            result.distance_sq = probe.distance_sq;
            result.dot = probe.dot;
        } else if (probe.distance < result.distance) {
            result.distance = probe.distance;
            result.distance_sq = probe.distance_sq;
            result.dot = probe.dot;
        }

        if (state == TDNE_VIS_VISIBLE) {
            result.samples_in_shape += 1;
            clear = tdne_line_clear(sensor, p, raycast, world_user, &hit);
            if (clear) {
                result.rays_clear += 1;
                result.last_seen_point = p;
            } else {
                result.rays_blocked += 1;
                if (result.hit.hit == TDNE_FALSE) {
                    result.hit = hit;
                }
            }
        } else if (first_reject == TDNE_VIS_NONE) {
            first_reject = state;
        }
    }

    if (result.rays_clear > 0) {
        result.visible = TDNE_TRUE;
        if (result.rays_clear == sample_total) {
            result.visibility = TDNE_VIS_VISIBLE;
            result.reason = TDNE_VIS_VISIBLE;
        } else {
            result.visibility = TDNE_VIS_PARTIAL;
            result.reason = TDNE_VIS_PARTIAL;
        }

        sample_score = tdne_mul_div_nonnegative_sat(result.rays_clear, TDNE_SCORE_SCALE, sample_total);
        dist_score = tdne_distance_score(result.distance, sensor->max_distance);
        result.score = (sample_score * 7 + dist_score * 3) / 10;
    } else if (result.samples_in_shape > 0) {
        result.visible = TDNE_FALSE;
        result.visibility = TDNE_VIS_OCCLUDED;
        result.reason = TDNE_VIS_OCCLUDED;
        result.score = 0;
    } else {
        result.visible = TDNE_FALSE;
        if (first_reject == TDNE_VIS_NONE) {
            first_reject = TDNE_VIS_OUT_OF_SHAPE;
        }
        result.visibility = first_reject;
        result.reason = first_reject;
        result.score = 0;
    }

    if (out_result != 0) {
        *out_result = result;
    }
    return result.visibility;
}

int tdne_scan_targets(
    const tdne_sensor *sensor,
    const tdne_target *targets,
    int target_count,
    tdne_result *results,
    int result_max,
    tdne_raycast_fn raycast,
    void *world_user,
    tdne_u32 flags
)
{
    int i;
    int count;
    int include_rejected;
    tdne_result tmp;

    if (sensor == 0 || targets == 0 || target_count <= 0 || results == 0 || result_max <= 0) {
        return 0;
    }

    include_rejected = (flags & TDNE_SCAN_KEEP_REJECTED) ? TDNE_TRUE : TDNE_FALSE;
    count = 0;

    for (i = 0; i < target_count; ++i) {
        tdne_eval_target(sensor, &targets[i], raycast, world_user, &tmp);
        tmp.target_index = i;

        if (include_rejected || tmp.visible) {
            if (count < result_max) {
                results[count] = tmp;
                count += 1;
            }
        }
    }

    if ((flags & TDNE_SCAN_SORT_BY_SCORE) != 0UL) {
        tdne_sort_results_by_score(results, count);
    }

    return count;
}

const char *tdne_visibility_name(int visibility)
{
    switch (visibility) {
        case TDNE_VIS_VISIBLE:
            return "visible";
        case TDNE_VIS_PARTIAL:
            return "partial";
        case TDNE_VIS_OCCLUDED:
            return "occluded";
        case TDNE_VIS_OUT_OF_RANGE:
            return "out_of_range";
        case TDNE_VIS_OUT_OF_SHAPE:
            return "out_of_shape";
        case TDNE_VIS_MASKED:
            return "masked";
        case TDNE_VIS_BAD_INPUT:
            return "bad_input";
        case TDNE_VIS_NONE:
        default:
            return "none";
    }
}
