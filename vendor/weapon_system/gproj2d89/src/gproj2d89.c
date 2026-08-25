#include "gproj2d89.h"

#define GP2D_ARRAY_COUNT(a) ((unsigned short)(sizeof(a) / sizeof((a)[0])))
#define GP2D_FRAC(n,d) ((gp2d_fx)(((n) * GP2D_FX_ONE) / (d)))

static const gp2d_vec2 gp2d_unit_circle_32[32] = {
    {65536L,0L},{64277L,12785L},{60547L,25080L},{54491L,36410L},
    {46341L,46341L},{36410L,54491L},{25080L,60547L},{12785L,64277L},
    {0L,65536L},{-12785L,64277L},{-25080L,60547L},{-36410L,54491L},
    {-46341L,46341L},{-54491L,36410L},{-60547L,25080L},{-64277L,12785L},
    {-65536L,0L},{-64277L,-12785L},{-60547L,-25080L},{-54491L,-36410L},
    {-46341L,-46341L},{-36410L,-54491L},{-25080L,-60547L},{-12785L,-64277L},
    {0L,-65536L},{12785L,-64277L},{25080L,-60547L},{36410L,-54491L},
    {46341L,-46341L},{54491L,-36410L},{60547L,-25080L},{64277L,-12785L}
};

static const gp2d_vec2 gp2d_rotation_64[64] = {
    {65536L,0L},{65220L,6424L},{64277L,12785L},{62714L,19024L},
    {60547L,25080L},{57798L,30893L},{54491L,36410L},{50660L,41576L},
    {46341L,46341L},{41576L,50660L},{36410L,54491L},{30893L,57798L},
    {25080L,60547L},{19024L,62714L},{12785L,64277L},{6424L,65220L},
    {0L,65536L},{-6424L,65220L},{-12785L,64277L},{-19024L,62714L},
    {-25080L,60547L},{-30893L,57798L},{-36410L,54491L},{-41576L,50660L},
    {-46341L,46341L},{-50660L,41576L},{-54491L,36410L},{-57798L,30893L},
    {-60547L,25080L},{-62714L,19024L},{-64277L,12785L},{-65220L,6424L},
    {-65536L,0L},{-65220L,-6424L},{-64277L,-12785L},{-62714L,-19024L},
    {-60547L,-25080L},{-57798L,-30893L},{-54491L,-36410L},{-50660L,-41576L},
    {-46341L,-46341L},{-41576L,-50660L},{-36410L,-54491L},{-30893L,-57798L},
    {-25080L,-60547L},{-19024L,-62714L},{-12785L,-64277L},{-6424L,-65220L},
    {0L,-65536L},{6424L,-65220L},{12785L,-64277L},{19024L,-62714L},
    {25080L,-60547L},{30893L,-57798L},{36410L,-54491L},{41576L,-50660L},
    {46341L,-46341L},{50660L,-41576L},{54491L,-36410L},{57798L,-30893L},
    {60547L,-25080L},{62714L,-19024L},{64277L,-12785L},{65220L,-6424L}
};

static const gp2d_profile gp2d_profiles[GP2D_AMMO_COUNT] = {
    {GP2D_AMMO_PISTOL, "pistol", "Pistola", GP2D_SHAPE_CASE_STRAIGHT,
     GP2D_SHAPE_DROPLET,
     GP2D_PROFILE_FLAG_HAS_SHELL | GP2D_PROFILE_FLAG_HAS_PROJECTILE},
    {GP2D_AMMO_SHOTGUN_BUCKSHOT, "shotgun_buckshot", "Escopeta / postas",
     GP2D_SHAPE_SHOT_SHELL, GP2D_SHAPE_CIRCLE,
     GP2D_PROFILE_FLAG_HAS_SHELL | GP2D_PROFILE_FLAG_HAS_PROJECTILE |
     GP2D_PROFILE_FLAG_MULTI_PATH},
    {GP2D_AMMO_SHOTGUN_SLUG, "shotgun_slug", "Escopeta / slug",
     GP2D_SHAPE_SHOT_SHELL, GP2D_SHAPE_FLAT_POINT,
     GP2D_PROFILE_FLAG_HAS_SHELL | GP2D_PROFILE_FLAG_HAS_PROJECTILE},
    {GP2D_AMMO_MACHINE_GUN, "machine_gun", "Ametralladora",
     GP2D_SHAPE_CASE_BOTTLENECK, GP2D_SHAPE_SPITZER,
     GP2D_PROFILE_FLAG_HAS_SHELL | GP2D_PROFILE_FLAG_HAS_PROJECTILE},
    {GP2D_AMMO_MAGNUM, "magnum", "Magnum", GP2D_SHAPE_CASE_STRAIGHT,
     GP2D_SHAPE_ROUND_NOSE,
     GP2D_PROFILE_FLAG_HAS_SHELL | GP2D_PROFILE_FLAG_HAS_PROJECTILE},
    {GP2D_AMMO_SNIPER, "sniper", "Rifle sniper",
     GP2D_SHAPE_CASE_BOTTLENECK, GP2D_SHAPE_SPITZER_BOATTAIL,
     GP2D_PROFILE_FLAG_HAS_SHELL | GP2D_PROFILE_FLAG_HAS_PROJECTILE},
    {GP2D_AMMO_MISSILE, "missile", "Misil", 0, GP2D_SHAPE_ROCKET,
     GP2D_PROFILE_FLAG_HAS_PROJECTILE | GP2D_PROFILE_FLAG_MULTI_PATH},
    {GP2D_AMMO_HAND_GRENADE, "hand_grenade", "Granada de mano", 0,
     GP2D_SHAPE_HAND_GRENADE,
     GP2D_PROFILE_FLAG_HAS_PROJECTILE | GP2D_PROFILE_FLAG_MULTI_PATH},
    {GP2D_AMMO_GRENADE_LAUNCHER, "grenade_launcher", "Lanzagranadas",
     GP2D_SHAPE_CASE_STRAIGHT, GP2D_SHAPE_40MM_GRENADE,
     GP2D_PROFILE_FLAG_HAS_SHELL | GP2D_PROFILE_FLAG_HAS_PROJECTILE |
     GP2D_PROFILE_FLAG_MULTI_PATH}
};

static unsigned long gp2d_magnitude(gp2d_fx value)
{
    if (value < 0L) {
        return (unsigned long)(-(value + 1L)) + 1UL;
    }
    return (unsigned long)value;
}

static gp2d_fx gp2d_from_magnitude(unsigned long value, int negative)
{
    if (negative) {
        if (value >= 2147483648UL) {
            return GP2D_FX_MIN;
        }
        return (gp2d_fx)(-(gp2d_fx)value);
    }
    if (value > 2147483647UL) {
        return GP2D_FX_MAX;
    }
    return (gp2d_fx)value;
}

static gp2d_fx gp2d_sat_add(gp2d_fx a, gp2d_fx b)
{
    if (b > 0L && a > GP2D_FX_MAX - b) {
        return GP2D_FX_MAX;
    }
    if (b < 0L && a < GP2D_FX_MIN - b) {
        return GP2D_FX_MIN;
    }
    return a + b;
}

static gp2d_fx gp2d_sat_sub(gp2d_fx a, gp2d_fx b)
{
    if (b == GP2D_FX_MIN) {
        if (a >= 0L) {
            return GP2D_FX_MAX;
        }
        return gp2d_sat_add(gp2d_sat_add(a, GP2D_FX_MAX), 1L);
    }
    return gp2d_sat_add(a, -b);
}

gp2d_fx gp2d_fx_from_int(long value)
{
    if (value > 32767L) {
        return GP2D_FX_MAX;
    }
    if (value < -32768L) {
        return GP2D_FX_MIN;
    }
    return value * GP2D_FX_ONE;
}

gp2d_fx gp2d_fx_from_ratio(long numerator, long denominator)
{
    unsigned long un;
    unsigned long ud;
    unsigned long whole;
    unsigned long rem;
    unsigned long frac;
    unsigned long bit;
    unsigned long result;
    unsigned long half_up;
    int negative;
    int i;

    if (denominator == 0L) {
        return numerator < 0L ? GP2D_FX_MIN : GP2D_FX_MAX;
    }
    negative = ((numerator < 0L) != (denominator < 0L));
    un = gp2d_magnitude(numerator);
    ud = gp2d_magnitude(denominator);
    whole = un / ud;
    rem = un % ud;
    if (whole > 32768UL || (!negative && whole > 32767UL)) {
        return negative ? GP2D_FX_MIN : GP2D_FX_MAX;
    }
    frac = 0UL;
    bit = 32768UL;
    for (i = 0; i < 16; ++i) {
        half_up = (ud >> 1) + (ud & 1UL);
        if (rem >= half_up) {
            rem = rem - (ud - rem);
            frac |= bit;
        } else {
            rem <<= 1;
        }
        bit >>= 1;
    }
    result = (whole << 16) | frac;
    return gp2d_from_magnitude(result, negative);
}

long gp2d_fx_to_int(gp2d_fx value)
{
    unsigned long magnitude;
    unsigned long rounded;

    if (value >= 0L) {
        return value / GP2D_FX_ONE;
    }
    magnitude = gp2d_magnitude(value);
    rounded = (magnitude + (unsigned long)GP2D_FX_ONE - 1UL) /
              (unsigned long)GP2D_FX_ONE;
    return -(long)rounded;
}

gp2d_fx gp2d_fx_mul(gp2d_fx a, gp2d_fx b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long a0;
    unsigned long a1;
    unsigned long b0;
    unsigned long b1;
    unsigned long p0;
    unsigned long p1;
    unsigned long p2;
    unsigned long p3;
    unsigned long result;
    unsigned long term;
    unsigned long limit;
    int negative;

    negative = ((a < 0L) != (b < 0L));
    ua = gp2d_magnitude(a);
    ub = gp2d_magnitude(b);
    a0 = ua & 65535UL;
    a1 = ua >> 16;
    b0 = ub & 65535UL;
    b1 = ub >> 16;
    p0 = a0 * b0;
    p1 = a0 * b1;
    p2 = a1 * b0;
    p3 = a1 * b1;
    limit = negative ? 2147483648UL : 2147483647UL;
    if (p3 > (limit >> 16)) {
        return negative ? GP2D_FX_MIN : GP2D_FX_MAX;
    }
    result = p3 << 16;
    term = p1;
    if (term > limit || result > limit - term) {
        return negative ? GP2D_FX_MIN : GP2D_FX_MAX;
    }
    result += term;
    term = p2;
    if (term > limit || result > limit - term) {
        return negative ? GP2D_FX_MIN : GP2D_FX_MAX;
    }
    result += term;
    term = p0 >> 16;
    if (term > limit || result > limit - term) {
        return negative ? GP2D_FX_MIN : GP2D_FX_MAX;
    }
    result += term;
    return gp2d_from_magnitude(result, negative);
}

gp2d_fx gp2d_fx_abs(gp2d_fx value)
{
    if (value == GP2D_FX_MIN) {
        return GP2D_FX_MAX;
    }
    return value < 0L ? -value : value;
}

gp2d_vec2 gp2d_vec2_make(gp2d_fx x, gp2d_fx y)
{
    gp2d_vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

gp2d_color gp2d_color_rgba(unsigned char r, unsigned char g,
                            unsigned char b, unsigned char a)
{
    gp2d_color result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = a;
    return result;
}

gp2d_transform gp2d_transform_identity(void)
{
    gp2d_transform result;
    result.position = gp2d_vec2_make(0L, 0L);
    result.scale = gp2d_vec2_make(GP2D_FX_ONE, GP2D_FX_ONE);
    result.rotation_step = 0U;
    return result;
}

gp2d_style gp2d_style_make(gp2d_color fill, gp2d_color outline,
                            gp2d_fx outline_width,
                            int fill_enabled, int outline_enabled)
{
    gp2d_style result;
    result.fill = fill;
    result.outline = outline;
    result.outline_width = outline_width;
    result.fill_enabled = fill_enabled ? GP2D_TRUE : GP2D_FALSE;
    result.outline_enabled = outline_enabled ? GP2D_TRUE : GP2D_FALSE;
    return result;
}

gp2d_draw_options gp2d_draw_options_default(void)
{
    gp2d_draw_options result;
    result.fill_override = gp2d_color_rgba(255U, 255U, 255U, 255U);
    result.outline_override = gp2d_color_rgba(24U, 27U, 31U, 255U);
    result.outline_width = GP2D_FRAC(3, 100);
    result.override_mask = 0U;
    result.outline_enabled = GP2D_TRUE;
    result.detail_enabled = GP2D_TRUE;
    return result;
}

void gp2d_scene_init(gp2d_scene *scene,
                     gp2d_path *paths, unsigned short path_capacity,
                     gp2d_vec2 *points, unsigned short point_capacity)
{
    if (scene == 0) {
        return;
    }
    scene->paths = paths;
    scene->points = points;
    scene->path_capacity = path_capacity;
    scene->point_capacity = point_capacity;
    scene->path_count = 0U;
    scene->point_count = 0U;
    scene->error = (paths == 0 || points == 0) ? GP2D_ERR_NULL : GP2D_OK;
}

void gp2d_scene_reset(gp2d_scene *scene)
{
    if (scene == 0) {
        return;
    }
    scene->path_count = 0U;
    scene->point_count = 0U;
    scene->error = (scene->paths == 0 || scene->points == 0) ?
                   GP2D_ERR_NULL : GP2D_OK;
}

int gp2d_scene_validate(const gp2d_scene *scene)
{
    unsigned short i;
    unsigned long end;
    const gp2d_path *path;

    if (scene == 0 || scene->paths == 0 || scene->points == 0) {
        return GP2D_ERR_NULL;
    }
    if (scene->path_count > scene->path_capacity ||
        scene->point_count > scene->point_capacity) {
        return GP2D_ERR_CORRUPT;
    }
    for (i = 0U; i < scene->path_count; ++i) {
        path = &scene->paths[i];
        end = (unsigned long)path->first_point +
              (unsigned long)path->point_count;
        if (end > (unsigned long)scene->point_count) {
            return GP2D_ERR_CORRUPT;
        }
        if (path->closed && path->point_count < 3U) {
            return GP2D_ERR_CORRUPT;
        }
    }
    return GP2D_OK;
}

static gp2d_vec2 gp2d_transform_point(const gp2d_transform *transform,
                                      gp2d_vec2 local)
{
    gp2d_vec2 result;
    gp2d_fx x;
    gp2d_fx y;
    gp2d_fx c;
    gp2d_fx s;
    unsigned char step;

    x = gp2d_fx_mul(local.x, transform->scale.x);
    y = gp2d_fx_mul(local.y, transform->scale.y);
    step = (unsigned char)(transform->rotation_step & 63U);
    c = gp2d_rotation_64[step].x;
    s = gp2d_rotation_64[step].y;
    result.x = gp2d_sat_sub(gp2d_fx_mul(x, c), gp2d_fx_mul(y, s));
    result.y = gp2d_sat_add(gp2d_fx_mul(x, s), gp2d_fx_mul(y, c));
    result.x = gp2d_sat_add(result.x, transform->position.x);
    result.y = gp2d_sat_add(result.y, transform->position.y);
    return result;
}

static gp2d_transform gp2d_child_transform(const gp2d_transform *parent,
                                           gp2d_fx offset_x,
                                           gp2d_fx offset_y,
                                           gp2d_fx scale_x,
                                           gp2d_fx scale_y)
{
    gp2d_transform child;
    gp2d_vec2 offset;

    child = *parent;
    offset = gp2d_vec2_make(offset_x, offset_y);
    child.position = gp2d_transform_point(parent, offset);
    child.scale.x = gp2d_fx_mul(parent->scale.x, scale_x);
    child.scale.y = gp2d_fx_mul(parent->scale.y, scale_y);
    return child;
}

static int gp2d_scene_reserve(gp2d_scene *scene, unsigned short points)
{
    unsigned long point_end;

    if (scene == 0 || scene->paths == 0 || scene->points == 0) {
        return GP2D_ERR_NULL;
    }
    if (scene->error != GP2D_OK) {
        return scene->error;
    }
    point_end = (unsigned long)scene->point_count + (unsigned long)points;
    if (scene->path_count >= scene->path_capacity ||
        point_end > (unsigned long)scene->point_capacity) {
        scene->error = GP2D_ERR_CAPACITY;
        return scene->error;
    }
    return GP2D_OK;
}

int gp2d_add_polygon(gp2d_scene *scene,
                     const gp2d_vec2 *local_points,
                     unsigned short point_count,
                     const gp2d_transform *transform,
                     const gp2d_style *style,
                     int closed, unsigned char role)
{
    gp2d_path *path;
    unsigned short first;
    unsigned short i;
    int status;

    if (scene == 0 || local_points == 0 || transform == 0 || style == 0) {
        return GP2D_ERR_NULL;
    }
    if (point_count == 0U || (closed && point_count < 3U)) {
        return GP2D_ERR_ARGUMENT;
    }
    status = gp2d_scene_reserve(scene, point_count);
    if (status != GP2D_OK) {
        return status;
    }
    first = scene->point_count;
    path = &scene->paths[scene->path_count];
    path->first_point = first;
    path->point_count = point_count;
    path->style = *style;
    path->closed = closed ? GP2D_TRUE : GP2D_FALSE;
    path->role = role;
    for (i = 0U; i < point_count; ++i) {
        scene->points[first + i] = gp2d_transform_point(transform,
                                                       local_points[i]);
    }
    scene->point_count = (unsigned short)(scene->point_count + point_count);
    scene->path_count = (unsigned short)(scene->path_count + 1U);
    return GP2D_OK;
}

int gp2d_add_circle(gp2d_scene *scene, gp2d_fx radius,
                    const gp2d_transform *transform,
                    const gp2d_style *style, unsigned char role)
{
    return gp2d_add_ellipse(scene, radius, radius, transform, style, role);
}

int gp2d_add_ellipse(gp2d_scene *scene, gp2d_fx radius_x, gp2d_fx radius_y,
                     const gp2d_transform *transform,
                     const gp2d_style *style, unsigned char role)
{
    gp2d_vec2 points[32];
    unsigned short i;

    if (radius_x <= 0L || radius_y <= 0L) {
        return GP2D_ERR_ARGUMENT;
    }
    for (i = 0U; i < 32U; ++i) {
        points[i].x = gp2d_fx_mul(gp2d_unit_circle_32[i].x, radius_x);
        points[i].y = gp2d_fx_mul(gp2d_unit_circle_32[i].y, radius_y);
    }
    return gp2d_add_polygon(scene, points, 32U, transform, style,
                            GP2D_TRUE, role);
}

int gp2d_add_capsule(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                     int axis,
                     const gp2d_transform *transform,
                     const gp2d_style *style, unsigned char role)
{
    gp2d_vec2 points[18];
    gp2d_fx radius;
    gp2d_fx half_straight;
    int i;
    int index;

    if (width <= 0L || height <= 0L) {
        return GP2D_ERR_ARGUMENT;
    }
    if (axis == GP2D_AXIS_VERTICAL) {
        if (height < width) {
            return GP2D_ERR_ARGUMENT;
        }
        radius = width / 2L;
        half_straight = (height - width) / 2L;
        index = 0;
        for (i = 0; i <= 8; ++i) {
            points[index].x = gp2d_fx_mul(gp2d_unit_circle_32[i * 2].x,
                                          radius);
            points[index].y = gp2d_sat_add(
                gp2d_fx_mul(gp2d_unit_circle_32[i * 2].y, radius),
                half_straight);
            ++index;
        }
        for (i = 16; i <= 32; i += 2) {
            int circle_index;
            circle_index = i & 31;
            points[index].x = gp2d_fx_mul(
                gp2d_unit_circle_32[circle_index].x, radius);
            points[index].y = gp2d_sat_sub(
                gp2d_fx_mul(gp2d_unit_circle_32[circle_index].y, radius),
                half_straight);
            ++index;
        }
    } else if (axis == GP2D_AXIS_HORIZONTAL) {
        if (width < height) {
            return GP2D_ERR_ARGUMENT;
        }
        radius = height / 2L;
        half_straight = (width - height) / 2L;
        index = 0;
        for (i = 24; i <= 40; i += 2) {
            int circle_index;
            circle_index = i & 31;
            points[index].x = gp2d_sat_add(
                gp2d_fx_mul(gp2d_unit_circle_32[circle_index].x, radius),
                half_straight);
            points[index].y = gp2d_fx_mul(
                gp2d_unit_circle_32[circle_index].y, radius);
            ++index;
        }
        for (i = 8; i <= 24; i += 2) {
            int circle_index;
            circle_index = i & 31;
            points[index].x = gp2d_sat_sub(
                gp2d_fx_mul(gp2d_unit_circle_32[circle_index].x, radius),
                half_straight);
            points[index].y = gp2d_fx_mul(
                gp2d_unit_circle_32[circle_index].y, radius);
            ++index;
        }
    } else {
        return GP2D_ERR_ARGUMENT;
    }
    return gp2d_add_polygon(scene, points, 18U, transform, style,
                            GP2D_TRUE, role);
}

int gp2d_add_droplet(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                     const gp2d_transform *transform,
                     const gp2d_style *style, unsigned char role)
{
    gp2d_vec2 points[14];
    gp2d_fx hw;
    gp2d_fx hh;

    if (width <= 0L || height <= 0L) {
        return GP2D_ERR_ARGUMENT;
    }
    hw = width / 2L;
    hh = height / 2L;
    points[0] = gp2d_vec2_make(0L, hh);
    points[1] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(1, 3)),
                               gp2d_fx_mul(hh, GP2D_FRAC(7, 10)));
    points[2] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(4, 5)),
                               gp2d_fx_mul(hh, GP2D_FRAC(2, 5)));
    points[3] = gp2d_vec2_make(hw, 0L);
    points[4] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(9, 10)),
                               -gp2d_fx_mul(hh, GP2D_FRAC(3, 5)));
    points[5] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(3, 5)),
                               -gp2d_fx_mul(hh, GP2D_FRAC(9, 10)));
    points[6] = gp2d_vec2_make(0L, -hh);
    points[7] = gp2d_vec2_make(-points[5].x, points[5].y);
    points[8] = gp2d_vec2_make(-points[4].x, points[4].y);
    points[9] = gp2d_vec2_make(-points[3].x, points[3].y);
    points[10] = gp2d_vec2_make(-points[2].x, points[2].y);
    points[11] = gp2d_vec2_make(-points[1].x, points[1].y);
    points[12] = gp2d_vec2_make(-gp2d_fx_mul(hw, GP2D_FRAC(1, 8)),
                                gp2d_fx_mul(hh, GP2D_FRAC(9, 10)));
    points[13] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(1, 8)),
                                gp2d_fx_mul(hh, GP2D_FRAC(9, 10)));
    return gp2d_add_polygon(scene, points, 14U, transform, style,
                            GP2D_TRUE, role);
}

int gp2d_add_round_nose(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                        const gp2d_transform *transform,
                        const gp2d_style *style, unsigned char role)
{
    gp2d_vec2 points[14];
    gp2d_fx hw;
    gp2d_fx hh;

    if (width <= 0L || height <= 0L) {
        return GP2D_ERR_ARGUMENT;
    }
    hw = width / 2L;
    hh = height / 2L;
    points[0] = gp2d_vec2_make(0L, hh);
    points[1] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(1, 2)),
                               gp2d_fx_mul(hh, GP2D_FRAC(9, 10)));
    points[2] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(4, 5)),
                               gp2d_fx_mul(hh, GP2D_FRAC(3, 5)));
    points[3] = gp2d_vec2_make(hw, gp2d_fx_mul(hh, GP2D_FRAC(1, 5)));
    points[4] = gp2d_vec2_make(hw, -gp2d_fx_mul(hh, GP2D_FRAC(4, 5)));
    points[5] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(9, 10)), -hh);
    points[6] = gp2d_vec2_make(0L, -hh);
    points[7] = gp2d_vec2_make(-points[5].x, points[5].y);
    points[8] = gp2d_vec2_make(-points[4].x, points[4].y);
    points[9] = gp2d_vec2_make(-points[3].x, points[3].y);
    points[10] = gp2d_vec2_make(-points[2].x, points[2].y);
    points[11] = gp2d_vec2_make(-points[1].x, points[1].y);
    points[12] = gp2d_vec2_make(-gp2d_fx_mul(hw, GP2D_FRAC(1, 4)),
                                gp2d_fx_mul(hh, GP2D_FRAC(49, 50)));
    points[13] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(1, 4)),
                                gp2d_fx_mul(hh, GP2D_FRAC(49, 50)));
    return gp2d_add_polygon(scene, points, 14U, transform, style,
                            GP2D_TRUE, role);
}

int gp2d_add_spitzer(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                     int boat_tail,
                     const gp2d_transform *transform,
                     const gp2d_style *style, unsigned char role)
{
    gp2d_vec2 points[12];
    gp2d_fx hw;
    gp2d_fx hh;
    gp2d_fx base;

    if (width <= 0L || height <= 0L) {
        return GP2D_ERR_ARGUMENT;
    }
    hw = width / 2L;
    hh = height / 2L;
    base = boat_tail ? gp2d_fx_mul(hw, GP2D_FRAC(3, 5)) : hw;
    points[0] = gp2d_vec2_make(0L, hh);
    points[1] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(1, 5)),
                               gp2d_fx_mul(hh, GP2D_FRAC(7, 10)));
    points[2] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(7, 10)),
                               gp2d_fx_mul(hh, GP2D_FRAC(1, 5)));
    points[3] = gp2d_vec2_make(hw, -gp2d_fx_mul(hh, GP2D_FRAC(1, 5)));
    points[4] = gp2d_vec2_make(hw, -gp2d_fx_mul(hh, GP2D_FRAC(4, 5)));
    points[5] = gp2d_vec2_make(base, -hh);
    points[6] = gp2d_vec2_make(-base, -hh);
    points[7] = gp2d_vec2_make(-points[4].x, points[4].y);
    points[8] = gp2d_vec2_make(-points[3].x, points[3].y);
    points[9] = gp2d_vec2_make(-points[2].x, points[2].y);
    points[10] = gp2d_vec2_make(-points[1].x, points[1].y);
    points[11] = gp2d_vec2_make(0L, hh);
    return gp2d_add_polygon(scene, points, 12U, transform, style,
                            GP2D_TRUE, role);
}

int gp2d_add_flat_point(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                        const gp2d_transform *transform,
                        const gp2d_style *style, unsigned char role)
{
    gp2d_vec2 points[10];
    gp2d_fx hw;
    gp2d_fx hh;
    gp2d_fx tip;

    if (width <= 0L || height <= 0L) {
        return GP2D_ERR_ARGUMENT;
    }
    hw = width / 2L;
    hh = height / 2L;
    tip = gp2d_fx_mul(hw, GP2D_FRAC(3, 5));
    points[0] = gp2d_vec2_make(-tip, hh);
    points[1] = gp2d_vec2_make(tip, hh);
    points[2] = gp2d_vec2_make(hw, gp2d_fx_mul(hh, GP2D_FRAC(1, 2)));
    points[3] = gp2d_vec2_make(hw, -gp2d_fx_mul(hh, GP2D_FRAC(4, 5)));
    points[4] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(9, 10)), -hh);
    points[5] = gp2d_vec2_make(-points[4].x, points[4].y);
    points[6] = gp2d_vec2_make(-points[3].x, points[3].y);
    points[7] = gp2d_vec2_make(-points[2].x, points[2].y);
    points[8] = gp2d_vec2_make(-tip, hh);
    points[9] = gp2d_vec2_make(0L, hh);
    return gp2d_add_polygon(scene, points, 10U, transform, style,
                            GP2D_TRUE, role);
}

static gp2d_style gp2d_resolve_style(gp2d_color base_fill,
                                     const gp2d_draw_options *options,
                                     int fill_enabled)
{
    gp2d_color outline;
    gp2d_color fill;
    gp2d_fx width;
    int outline_enabled;

    fill = base_fill;
    outline = gp2d_color_rgba(24U, 27U, 31U, 255U);
    width = GP2D_FRAC(3, 100);
    outline_enabled = GP2D_TRUE;
    if (options != 0) {
        if ((options->override_mask & GP2D_OVERRIDE_FILL) != 0U) {
            fill = options->fill_override;
        }
        if ((options->override_mask & GP2D_OVERRIDE_OUTLINE) != 0U) {
            outline = options->outline_override;
        }
        width = options->outline_width;
        outline_enabled = options->outline_enabled;
    }
    return gp2d_style_make(fill, outline, width, fill_enabled,
                           outline_enabled);
}

static int gp2d_detail_enabled(const gp2d_draw_options *options)
{
    if (options == 0) {
        return GP2D_TRUE;
    }
    return options->detail_enabled ? GP2D_TRUE : GP2D_FALSE;
}

static int gp2d_add_box(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                        gp2d_fx offset_x, gp2d_fx offset_y,
                        const gp2d_transform *transform,
                        const gp2d_style *style, unsigned char role)
{
    gp2d_vec2 points[4];
    gp2d_fx hw;
    gp2d_fx hh;

    hw = width / 2L;
    hh = height / 2L;
    points[0] = gp2d_vec2_make(offset_x - hw, offset_y - hh);
    points[1] = gp2d_vec2_make(offset_x + hw, offset_y - hh);
    points[2] = gp2d_vec2_make(offset_x + hw, offset_y + hh);
    points[3] = gp2d_vec2_make(offset_x - hw, offset_y + hh);
    return gp2d_add_polygon(scene, points, 4U, transform, style,
                            GP2D_TRUE, role);
}

static int gp2d_add_line(gp2d_scene *scene,
                         gp2d_fx x0, gp2d_fx y0,
                         gp2d_fx x1, gp2d_fx y1,
                         const gp2d_transform *transform,
                         const gp2d_style *style, unsigned char role)
{
    gp2d_vec2 points[2];
    points[0] = gp2d_vec2_make(x0, y0);
    points[1] = gp2d_vec2_make(x1, y1);
    return gp2d_add_polygon(scene, points, 2U, transform, style,
                            GP2D_FALSE, role);
}

static int gp2d_build_straight_case(gp2d_scene *scene,
                                    gp2d_fx width, gp2d_fx height,
                                    int rimmed,
                                    const gp2d_transform *transform,
                                    const gp2d_draw_options *options)
{
    gp2d_vec2 body[6];
    gp2d_style brass;
    gp2d_style rim;
    gp2d_style primer;
    gp2d_style detail;
    gp2d_fx hw;
    gp2d_fx hh;
    gp2d_fx rim_h;
    gp2d_fx body_bottom;
    int status;

    brass = gp2d_resolve_style(gp2d_color_rgba(198U, 151U, 55U, 255U),
                               options, GP2D_TRUE);
    rim = gp2d_resolve_style(gp2d_color_rgba(151U, 105U, 36U, 255U),
                             options, GP2D_TRUE);
    primer = gp2d_resolve_style(gp2d_color_rgba(166U, 172U, 176U, 255U),
                                options, GP2D_TRUE);
    detail = gp2d_resolve_style(gp2d_color_rgba(116U, 78U, 27U, 255U),
                                options, GP2D_FALSE);
    hw = width / 2L;
    hh = height / 2L;
    rim_h = gp2d_fx_mul(height, GP2D_FRAC(13, 100));
    body_bottom = -hh + rim_h;
    body[0] = gp2d_vec2_make(-gp2d_fx_mul(hw, GP2D_FRAC(49, 50)),
                             body_bottom);
    body[1] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(49, 50)),
                             body_bottom);
    body[2] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(47, 50)), hh);
    body[3] = gp2d_vec2_make(-gp2d_fx_mul(hw, GP2D_FRAC(47, 50)), hh);
    body[4] = gp2d_vec2_make(-hw, gp2d_fx_mul(hh, GP2D_FRAC(3, 5)));
    body[5] = gp2d_vec2_make(-hw, body_bottom);
    status = gp2d_add_polygon(scene, body, 6U, transform, &brass,
                              GP2D_TRUE, GP2D_ROLE_SHELL_BODY);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene,
                          gp2d_fx_mul(width, rimmed ? GP2D_FRAC(23, 20) :
                                                   GP2D_FRAC(21, 20)),
                          rim_h, 0L, -hh + rim_h / 2L,
                          transform, &rim, GP2D_ROLE_SHELL_RIM);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene, gp2d_fx_mul(width, GP2D_FRAC(2, 5)),
                          gp2d_fx_mul(rim_h, GP2D_FRAC(2, 5)),
                          0L, -hh + gp2d_fx_mul(rim_h, GP2D_FRAC(1, 4)),
                          transform, &primer, GP2D_ROLE_PRIMER);
    if (status != GP2D_OK) {
        return status;
    }
    if (gp2d_detail_enabled(options)) {
        status = gp2d_add_line(scene, -gp2d_fx_mul(hw, GP2D_FRAC(9, 10)),
                               gp2d_fx_mul(hh, GP2D_FRAC(3, 4)),
                               gp2d_fx_mul(hw, GP2D_FRAC(9, 10)),
                               gp2d_fx_mul(hh, GP2D_FRAC(3, 4)),
                               transform, &detail, GP2D_ROLE_DETAIL);
    }
    return status;
}

static int gp2d_build_bottleneck_case(gp2d_scene *scene,
                                      gp2d_fx width, gp2d_fx height,
                                      const gp2d_transform *transform,
                                      const gp2d_draw_options *options)
{
    gp2d_vec2 body[10];
    gp2d_style brass;
    gp2d_style rim;
    gp2d_style primer;
    gp2d_style detail;
    gp2d_fx hw;
    gp2d_fx hh;
    gp2d_fx neck;
    gp2d_fx rim_h;
    int status;

    brass = gp2d_resolve_style(gp2d_color_rgba(202U, 156U, 61U, 255U),
                               options, GP2D_TRUE);
    rim = gp2d_resolve_style(gp2d_color_rgba(150U, 104U, 35U, 255U),
                             options, GP2D_TRUE);
    primer = gp2d_resolve_style(gp2d_color_rgba(168U, 174U, 179U, 255U),
                                options, GP2D_TRUE);
    detail = gp2d_resolve_style(gp2d_color_rgba(119U, 82U, 31U, 255U),
                                options, GP2D_FALSE);
    hw = width / 2L;
    hh = height / 2L;
    neck = gp2d_fx_mul(hw, GP2D_FRAC(11, 20));
    rim_h = gp2d_fx_mul(height, GP2D_FRAC(9, 100));
    body[0] = gp2d_vec2_make(-gp2d_fx_mul(hw, GP2D_FRAC(19, 20)),
                             -hh + rim_h);
    body[1] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(19, 20)),
                             -hh + rim_h);
    body[2] = gp2d_vec2_make(hw, gp2d_fx_mul(hh, GP2D_FRAC(1, 5)));
    body[3] = gp2d_vec2_make(gp2d_fx_mul(hw, GP2D_FRAC(9, 10)),
                             gp2d_fx_mul(hh, GP2D_FRAC(2, 5)));
    body[4] = gp2d_vec2_make(neck, gp2d_fx_mul(hh, GP2D_FRAC(7, 10)));
    body[5] = gp2d_vec2_make(neck, hh);
    body[6] = gp2d_vec2_make(-neck, hh);
    body[7] = gp2d_vec2_make(-body[4].x, body[4].y);
    body[8] = gp2d_vec2_make(-body[3].x, body[3].y);
    body[9] = gp2d_vec2_make(-body[2].x, body[2].y);
    status = gp2d_add_polygon(scene, body, 10U, transform, &brass,
                              GP2D_TRUE, GP2D_ROLE_SHELL_BODY);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene, gp2d_fx_mul(width, GP2D_FRAC(21, 20)),
                          rim_h, 0L, -hh + rim_h / 2L,
                          transform, &rim, GP2D_ROLE_SHELL_RIM);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene, gp2d_fx_mul(width, GP2D_FRAC(7, 20)),
                          gp2d_fx_mul(rim_h, GP2D_FRAC(2, 5)),
                          0L, -hh + gp2d_fx_mul(rim_h, GP2D_FRAC(1, 4)),
                          transform, &primer, GP2D_ROLE_PRIMER);
    if (status != GP2D_OK) {
        return status;
    }
    if (gp2d_detail_enabled(options)) {
        status = gp2d_add_line(scene, -neck,
                               gp2d_fx_mul(hh, GP2D_FRAC(4, 5)),
                               neck,
                               gp2d_fx_mul(hh, GP2D_FRAC(4, 5)),
                               transform, &detail, GP2D_ROLE_DETAIL);
    }
    return status;
}

static int gp2d_build_shotshell(gp2d_scene *scene,
                                const gp2d_transform *transform,
                                const gp2d_draw_options *options)
{
    gp2d_style hull;
    gp2d_style base;
    gp2d_style primer;
    gp2d_fx width;
    gp2d_fx height;
    gp2d_fx hh;
    gp2d_fx base_h;
    gp2d_transform child;
    int status;

    width = GP2D_FRAC(11, 20);
    height = GP2D_FRAC(7, 5);
    hh = height / 2L;
    base_h = gp2d_fx_mul(height, GP2D_FRAC(1, 5));

    /* Deliberately block-shaped shotgun shell: red hull, yellow base. */
    hull = gp2d_resolve_style(gp2d_color_rgba(214U, 42U, 42U, 255U),
                              options, GP2D_TRUE);
    base = gp2d_resolve_style(gp2d_color_rgba(255U, 211U, 48U, 255U),
                              options, GP2D_TRUE);
    primer = gp2d_resolve_style(gp2d_color_rgba(171U, 177U, 181U, 255U),
                                options, GP2D_TRUE);

    child = gp2d_child_transform(transform, 0L, base_h / 2L,
                                 GP2D_FX_ONE, GP2D_FX_ONE);
    status = gp2d_add_box(scene, width, height - base_h,
                          0L, 0L, &child, &hull, GP2D_ROLE_HULL);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene, width, base_h,
                          0L, -hh + base_h / 2L,
                          transform, &base, GP2D_ROLE_SHELL_RIM);
    if (status != GP2D_OK) {
        return status;
    }
    return gp2d_add_box(scene, gp2d_fx_mul(width, GP2D_FRAC(2, 5)),
                        gp2d_fx_mul(base_h, GP2D_FRAC(1, 3)),
                        0L, -hh + gp2d_fx_mul(base_h, GP2D_FRAC(1, 5)),
                        transform, &primer, GP2D_ROLE_PRIMER);
}

static int gp2d_build_buckshot(gp2d_scene *scene,
                               const gp2d_transform *transform,
                               const gp2d_draw_options *options)
{
    static const signed char offsets[9][2] = {
        {-1, 1}, {0, 1}, {1, 1},
        {-1, 0}, {0, 0}, {1, 0},
        {-1,-1}, {0,-1}, {1,-1}
    };
    gp2d_style pellet;
    gp2d_transform child;
    gp2d_fx spacing;
    gp2d_fx radius;
    int i;
    int status;

    pellet = gp2d_resolve_style(gp2d_color_rgba(116U, 121U, 126U, 255U),
                                options, GP2D_TRUE);
    spacing = GP2D_FRAC(9, 50);
    radius = GP2D_FRAC(2, 25);
    for (i = 0; i < 9; ++i) {
        child = gp2d_child_transform(transform,
                                    spacing * (gp2d_fx)offsets[i][0],
                                    spacing * (gp2d_fx)offsets[i][1],
                                    GP2D_FX_ONE, GP2D_FX_ONE);
        status = gp2d_add_circle(scene, radius, &child, &pellet,
                                 GP2D_ROLE_PELLET);
        if (status != GP2D_OK) {
            return status;
        }
    }
    return GP2D_OK;
}

static int gp2d_build_projectile_basic(gp2d_scene *scene, int ammo_id,
                                       const gp2d_transform *transform,
                                       const gp2d_draw_options *options)
{
    gp2d_style copper;
    gp2d_style jacket;
    gp2d_style lead;
    gp2d_style band;
    int status;

    copper = gp2d_resolve_style(gp2d_color_rgba(181U, 98U, 54U, 255U),
                                options, GP2D_TRUE);
    jacket = gp2d_resolve_style(gp2d_color_rgba(191U, 111U, 63U, 255U),
                                options, GP2D_TRUE);
    lead = gp2d_resolve_style(gp2d_color_rgba(113U, 119U, 125U, 255U),
                              options, GP2D_TRUE);
    band = gp2d_resolve_style(gp2d_color_rgba(126U, 67U, 39U, 255U),
                              options, GP2D_TRUE);
    status = GP2D_ERR_UNSUPPORTED;
    if (ammo_id == GP2D_AMMO_PISTOL) {
        status = gp2d_add_droplet(scene, GP2D_FRAC(9, 20),
                                  GP2D_FRAC(3, 5), transform, &copper,
                                  GP2D_ROLE_PROJECTILE);
    } else if (ammo_id == GP2D_AMMO_SHOTGUN_BUCKSHOT) {
        status = gp2d_build_buckshot(scene, transform, options);
    } else if (ammo_id == GP2D_AMMO_SHOTGUN_SLUG) {
        status = gp2d_add_flat_point(scene, GP2D_FRAC(11, 20),
                                     GP2D_FRAC(7, 10), transform, &lead,
                                     GP2D_ROLE_PROJECTILE);
    } else if (ammo_id == GP2D_AMMO_MACHINE_GUN) {
        status = gp2d_add_spitzer(scene, GP2D_FRAC(7, 20),
                                  GP2D_FRAC(17, 20), GP2D_FALSE,
                                  transform, &jacket, GP2D_ROLE_PROJECTILE);
        if (status == GP2D_OK && gp2d_detail_enabled(options)) {
            status = gp2d_add_box(scene, GP2D_FRAC(17, 50),
                                  GP2D_FRAC(1, 20), 0L,
                                  -GP2D_FRAC(1, 4), transform, &band,
                                  GP2D_ROLE_BAND);
        }
    } else if (ammo_id == GP2D_AMMO_MAGNUM) {
        status = gp2d_add_round_nose(scene, GP2D_FRAC(1, 2),
                                     GP2D_FRAC(7, 10), transform, &copper,
                                     GP2D_ROLE_PROJECTILE);
        if (status == GP2D_OK && gp2d_detail_enabled(options)) {
            status = gp2d_add_box(scene, GP2D_FRAC(23, 50),
                                  GP2D_FRAC(3, 50), 0L,
                                  -GP2D_FRAC(1, 5), transform, &band,
                                  GP2D_ROLE_BAND);
        }
    } else if (ammo_id == GP2D_AMMO_SNIPER) {
        status = gp2d_add_spitzer(scene, GP2D_FRAC(3, 10),
                                  GP2D_FRAC(6, 5), GP2D_TRUE,
                                  transform, &jacket, GP2D_ROLE_JACKET);
        if (status == GP2D_OK && gp2d_detail_enabled(options)) {
            status = gp2d_add_box(scene, GP2D_FRAC(7, 25),
                                  GP2D_FRAC(1, 25), 0L,
                                  -GP2D_FRAC(7, 20), transform, &band,
                                  GP2D_ROLE_BAND);
        }
    }
    return status;
}

static int gp2d_build_missile(gp2d_scene *scene,
                              const gp2d_transform *transform,
                              const gp2d_draw_options *options)
{
    gp2d_vec2 nose_points[8];
    gp2d_vec2 fin_left[4];
    gp2d_vec2 fin_right[4];
    gp2d_style body;
    gp2d_style nose;
    gp2d_style fin;
    gp2d_style nozzle;
    gp2d_style band;
    gp2d_transform child;
    int status;

    body = gp2d_resolve_style(gp2d_color_rgba(94U, 107U, 97U, 255U),
                              options, GP2D_TRUE);
    nose = gp2d_resolve_style(gp2d_color_rgba(66U, 75U, 70U, 255U),
                              options, GP2D_TRUE);
    fin = gp2d_resolve_style(gp2d_color_rgba(74U, 85U, 78U, 255U),
                             options, GP2D_TRUE);
    nozzle = gp2d_resolve_style(gp2d_color_rgba(48U, 52U, 55U, 255U),
                                options, GP2D_TRUE);
    band = gp2d_resolve_style(gp2d_color_rgba(196U, 176U, 78U, 255U),
                              options, GP2D_TRUE);
    child = gp2d_child_transform(transform, 0L, -GP2D_FRAC(1, 10),
                                 GP2D_FX_ONE, GP2D_FX_ONE);
    status = gp2d_add_capsule(scene, GP2D_FRAC(9, 20), GP2D_FRAC(7, 5),
                              GP2D_AXIS_VERTICAL, &child,
                              &body, GP2D_ROLE_PROJECTILE);
    if (status != GP2D_OK) {
        return status;
    }
    nose_points[0] = gp2d_vec2_make(0L, GP2D_FRAC(1, 1));
    nose_points[1] = gp2d_vec2_make(GP2D_FRAC(1, 20), GP2D_FRAC(19, 20));
    nose_points[2] = gp2d_vec2_make(GP2D_FRAC(3, 20), GP2D_FRAC(4, 5));
    nose_points[3] = gp2d_vec2_make(GP2D_FRAC(9, 40), GP2D_FRAC(3, 5));
    nose_points[4] = gp2d_vec2_make(-GP2D_FRAC(9, 40), GP2D_FRAC(3, 5));
    nose_points[5] = gp2d_vec2_make(-GP2D_FRAC(3, 20), GP2D_FRAC(4, 5));
    nose_points[6] = gp2d_vec2_make(-GP2D_FRAC(1, 20), GP2D_FRAC(19, 20));
    nose_points[7] = gp2d_vec2_make(0L, GP2D_FRAC(1, 1));
    status = gp2d_add_polygon(scene, nose_points, 8U, transform, &nose,
                              GP2D_TRUE, GP2D_ROLE_PROJECTILE);
    if (status != GP2D_OK) {
        return status;
    }
    fin_left[0] = gp2d_vec2_make(-GP2D_FRAC(9, 40), -GP2D_FRAC(2, 5));
    fin_left[1] = gp2d_vec2_make(-GP2D_FRAC(11, 20), -GP2D_FRAC(4, 5));
    fin_left[2] = gp2d_vec2_make(-GP2D_FRAC(9, 20), -GP2D_FRAC(9, 10));
    fin_left[3] = gp2d_vec2_make(-GP2D_FRAC(9, 40), -GP2D_FRAC(13, 20));
    fin_right[0] = gp2d_vec2_make(-fin_left[0].x, fin_left[0].y);
    fin_right[1] = gp2d_vec2_make(-fin_left[1].x, fin_left[1].y);
    fin_right[2] = gp2d_vec2_make(-fin_left[2].x, fin_left[2].y);
    fin_right[3] = gp2d_vec2_make(-fin_left[3].x, fin_left[3].y);
    status = gp2d_add_polygon(scene, fin_left, 4U, transform, &fin,
                              GP2D_TRUE, GP2D_ROLE_FIN);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_polygon(scene, fin_right, 4U, transform, &fin,
                              GP2D_TRUE, GP2D_ROLE_FIN);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene, GP2D_FRAC(9, 20), GP2D_FRAC(1, 12),
                          0L, GP2D_FRAC(9, 20), transform, &band,
                          GP2D_ROLE_BAND);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene, GP2D_FRAC(1, 4), GP2D_FRAC(3, 20),
                          0L, -GP2D_FRAC(17, 20), transform, &nozzle,
                          GP2D_ROLE_NOZZLE);
    return status;
}

static int gp2d_build_hand_grenade(gp2d_scene *scene,
                                   const gp2d_transform *transform,
                                   const gp2d_draw_options *options)
{
    gp2d_vec2 lever_points[5];
    gp2d_style body;
    gp2d_style fuze;
    gp2d_style lever;
    gp2d_style pin;
    gp2d_style detail;
    gp2d_transform child;
    int status;

    body = gp2d_resolve_style(gp2d_color_rgba(84U, 96U, 56U, 255U),
                              options, GP2D_TRUE);
    fuze = gp2d_resolve_style(gp2d_color_rgba(72U, 76U, 68U, 255U),
                              options, GP2D_TRUE);
    lever = gp2d_resolve_style(gp2d_color_rgba(132U, 126U, 94U, 255U),
                               options, GP2D_TRUE);
    pin = gp2d_resolve_style(gp2d_color_rgba(164U, 168U, 166U, 255U),
                             options, GP2D_FALSE);
    detail = gp2d_resolve_style(gp2d_color_rgba(49U, 58U, 36U, 255U),
                                options, GP2D_FALSE);
    child = gp2d_child_transform(transform, 0L, -GP2D_FRAC(1, 10),
                                 GP2D_FX_ONE, GP2D_FX_ONE);
    status = gp2d_add_ellipse(scene, GP2D_FRAC(11, 20), GP2D_FRAC(3, 5),
                              &child, &body, GP2D_ROLE_PROJECTILE);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene, GP2D_FRAC(7, 20), GP2D_FRAC(1, 5),
                          0L, GP2D_FRAC(13, 25), transform, &fuze,
                          GP2D_ROLE_FUZE);
    if (status != GP2D_OK) {
        return status;
    }
    lever_points[0] = gp2d_vec2_make(-GP2D_FRAC(3, 20), GP2D_FRAC(3, 5));
    lever_points[1] = gp2d_vec2_make(GP2D_FRAC(1, 4), GP2D_FRAC(3, 5));
    lever_points[2] = gp2d_vec2_make(GP2D_FRAC(9, 20), GP2D_FRAC(1, 5));
    lever_points[3] = gp2d_vec2_make(GP2D_FRAC(7, 20), GP2D_FRAC(3, 20));
    lever_points[4] = gp2d_vec2_make(GP2D_FRAC(1, 20), GP2D_FRAC(9, 20));
    status = gp2d_add_polygon(scene, lever_points, 5U, transform, &lever,
                              GP2D_TRUE, GP2D_ROLE_LEVER);
    if (status != GP2D_OK) {
        return status;
    }
    child = gp2d_child_transform(transform, GP2D_FRAC(9, 20),
                                 GP2D_FRAC(9, 20),
                                 GP2D_FX_ONE, GP2D_FX_ONE);
    status = gp2d_add_circle(scene, GP2D_FRAC(3, 20), &child, &pin,
                             GP2D_ROLE_PIN);
    if (status != GP2D_OK) {
        return status;
    }
    if (gp2d_detail_enabled(options)) {
        status = gp2d_add_line(scene, -GP2D_FRAC(9, 20),
                               -GP2D_FRAC(1, 10),
                               GP2D_FRAC(9, 20),
                               -GP2D_FRAC(1, 10),
                               transform, &detail, GP2D_ROLE_DETAIL);
        if (status != GP2D_OK) {
            return status;
        }
        status = gp2d_add_line(scene, 0L, -GP2D_FRAC(13, 20),
                               0L, GP2D_FRAC(2, 5),
                               transform, &detail, GP2D_ROLE_DETAIL);
    }
    return status;
}

static int gp2d_build_40mm_projectile(gp2d_scene *scene,
                                      const gp2d_transform *transform,
                                      const gp2d_draw_options *options)
{
    gp2d_style body;
    gp2d_style cap;
    gp2d_style band;
    gp2d_style base;
    gp2d_transform child;
    int status;

    body = gp2d_resolve_style(gp2d_color_rgba(88U, 102U, 61U, 255U),
                              options, GP2D_TRUE);
    cap = gp2d_resolve_style(gp2d_color_rgba(67U, 78U, 48U, 255U),
                             options, GP2D_TRUE);
    band = gp2d_resolve_style(gp2d_color_rgba(181U, 132U, 46U, 255U),
                              options, GP2D_TRUE);
    base = gp2d_resolve_style(gp2d_color_rgba(69U, 72U, 67U, 255U),
                              options, GP2D_TRUE);
    child = gp2d_child_transform(transform, 0L, GP2D_FRAC(1, 10),
                                 GP2D_FX_ONE, GP2D_FX_ONE);
    status = gp2d_add_capsule(scene, GP2D_FRAC(7, 10), GP2D_FRAC(9, 10),
                              GP2D_AXIS_VERTICAL, &child, &body,
                              GP2D_ROLE_PROJECTILE);
    if (status != GP2D_OK) {
        return status;
    }
    child = gp2d_child_transform(transform, 0L, GP2D_FRAC(11, 20),
                                 GP2D_FX_ONE, GP2D_FX_ONE);
    status = gp2d_add_ellipse(scene, GP2D_FRAC(7, 20), GP2D_FRAC(1, 5),
                              &child, &cap, GP2D_ROLE_PROJECTILE);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene, GP2D_FRAC(18, 25), GP2D_FRAC(2, 25),
                          0L, -GP2D_FRAC(7, 20), transform, &band,
                          GP2D_ROLE_BAND);
    if (status != GP2D_OK) {
        return status;
    }
    status = gp2d_add_box(scene, GP2D_FRAC(3, 5), GP2D_FRAC(3, 20),
                          0L, -GP2D_FRAC(1, 2), transform, &base,
                          GP2D_ROLE_SHELL_RIM);
    return status;
}

static int gp2d_build_shell(gp2d_scene *scene, int ammo_id,
                            const gp2d_transform *transform,
                            const gp2d_draw_options *options)
{
    if (ammo_id == GP2D_AMMO_PISTOL) {
        return gp2d_build_straight_case(scene, GP2D_FRAC(1, 2),
                                        GP2D_FRAC(6, 5), GP2D_FALSE,
                                        transform, options);
    }
    if (ammo_id == GP2D_AMMO_SHOTGUN_BUCKSHOT ||
        ammo_id == GP2D_AMMO_SHOTGUN_SLUG) {
        return gp2d_build_shotshell(scene, transform, options);
    }
    if (ammo_id == GP2D_AMMO_MACHINE_GUN) {
        return gp2d_build_bottleneck_case(scene, GP2D_FRAC(11, 20),
                                          GP2D_FRAC(8, 5), transform,
                                          options);
    }
    if (ammo_id == GP2D_AMMO_MAGNUM) {
        return gp2d_build_straight_case(scene, GP2D_FRAC(11, 20),
                                        GP2D_FRAC(8, 5), GP2D_TRUE,
                                        transform, options);
    }
    if (ammo_id == GP2D_AMMO_SNIPER) {
        return gp2d_build_bottleneck_case(scene, GP2D_FRAC(3, 5),
                                          GP2D_FRAC(19, 10), transform,
                                          options);
    }
    if (ammo_id == GP2D_AMMO_GRENADE_LAUNCHER) {
        return gp2d_build_straight_case(scene, GP2D_FRAC(4, 5),
                                        GP2D_FRAC(9, 10), GP2D_FALSE,
                                        transform, options);
    }
    return GP2D_ERR_UNSUPPORTED;
}

static int gp2d_build_projectile(gp2d_scene *scene, int ammo_id,
                                 const gp2d_transform *transform,
                                 const gp2d_draw_options *options)
{
    if (ammo_id >= GP2D_AMMO_PISTOL && ammo_id <= GP2D_AMMO_SNIPER) {
        return gp2d_build_projectile_basic(scene, ammo_id, transform,
                                           options);
    }
    if (ammo_id == GP2D_AMMO_MISSILE) {
        return gp2d_build_missile(scene, transform, options);
    }
    if (ammo_id == GP2D_AMMO_HAND_GRENADE) {
        return gp2d_build_hand_grenade(scene, transform, options);
    }
    if (ammo_id == GP2D_AMMO_GRENADE_LAUNCHER) {
        return gp2d_build_40mm_projectile(scene, transform, options);
    }
    return GP2D_ERR_UNSUPPORTED;
}

unsigned short gp2d_profile_count(void)
{
    return GP2D_AMMO_COUNT;
}

const gp2d_profile *gp2d_profile_at(unsigned short index)
{
    if (index >= GP2D_AMMO_COUNT) {
        return 0;
    }
    return &gp2d_profiles[index];
}

const gp2d_profile *gp2d_profile_get(int ammo_id)
{
    unsigned short i;
    for (i = 0U; i < GP2D_AMMO_COUNT; ++i) {
        if (gp2d_profiles[i].ammo_id == ammo_id) {
            return &gp2d_profiles[i];
        }
    }
    return 0;
}

int gp2d_profile_has_part(int ammo_id, int part)
{
    const gp2d_profile *profile;
    profile = gp2d_profile_get(ammo_id);
    if (profile == 0) {
        return GP2D_FALSE;
    }
    if (part == GP2D_PART_SHELL) {
        return (profile->flags & GP2D_PROFILE_FLAG_HAS_SHELL) != 0U;
    }
    if (part == GP2D_PART_PROJECTILE) {
        return (profile->flags & GP2D_PROFILE_FLAG_HAS_PROJECTILE) != 0U;
    }
    if (part == GP2D_PART_COMPLETE) {
        return (profile->flags & GP2D_PROFILE_FLAG_HAS_PROJECTILE) != 0U;
    }
    return GP2D_FALSE;
}

int gp2d_build_part(gp2d_scene *scene, int ammo_id, int part,
                    const gp2d_transform *transform,
                    const gp2d_draw_options *options)
{
    const gp2d_profile *profile;
    gp2d_transform shell_transform;
    gp2d_transform projectile_transform;
    int status;

    if (scene == 0 || transform == 0) {
        return GP2D_ERR_NULL;
    }
    profile = gp2d_profile_get(ammo_id);
    if (profile == 0) {
        return GP2D_ERR_ARGUMENT;
    }
    if (part == GP2D_PART_SHELL) {
        if ((profile->flags & GP2D_PROFILE_FLAG_HAS_SHELL) == 0U) {
            return GP2D_ERR_UNSUPPORTED;
        }
        return gp2d_build_shell(scene, ammo_id, transform, options);
    }
    if (part == GP2D_PART_PROJECTILE) {
        if ((profile->flags & GP2D_PROFILE_FLAG_HAS_PROJECTILE) == 0U) {
            return GP2D_ERR_UNSUPPORTED;
        }
        return gp2d_build_projectile(scene, ammo_id, transform, options);
    }
    if (part != GP2D_PART_COMPLETE) {
        return GP2D_ERR_ARGUMENT;
    }
    if ((profile->flags & GP2D_PROFILE_FLAG_HAS_SHELL) == 0U) {
        return gp2d_build_projectile(scene, ammo_id, transform, options);
    }
    shell_transform = gp2d_child_transform(transform, 0L,
                                            -GP2D_FRAC(2, 5),
                                            GP2D_FX_ONE, GP2D_FX_ONE);
    projectile_transform = gp2d_child_transform(transform, 0L,
                                                 GP2D_FRAC(3, 5),
                                                 GP2D_FX_ONE, GP2D_FX_ONE);
    status = gp2d_build_shell(scene, ammo_id, &shell_transform, options);
    if (status != GP2D_OK) {
        return status;
    }
    return gp2d_build_projectile(scene, ammo_id, &projectile_transform,
                                 options);
}

void gp2d_scene_recolor_all(gp2d_scene *scene, gp2d_color color)
{
    unsigned short i;
    if (scene == 0 || scene->paths == 0) {
        return;
    }
    for (i = 0U; i < scene->path_count; ++i) {
        if (scene->paths[i].style.fill_enabled) {
            scene->paths[i].style.fill = color;
        }
    }
}

void gp2d_scene_recolor_role(gp2d_scene *scene, unsigned char role,
                             gp2d_color color)
{
    unsigned short i;
    if (scene == 0 || scene->paths == 0) {
        return;
    }
    for (i = 0U; i < scene->path_count; ++i) {
        if (scene->paths[i].role == role &&
            scene->paths[i].style.fill_enabled) {
            scene->paths[i].style.fill = color;
        }
    }
}

void gp2d_scene_set_outline(gp2d_scene *scene, int enabled,
                            gp2d_color color, gp2d_fx width)
{
    unsigned short i;
    if (scene == 0 || scene->paths == 0) {
        return;
    }
    for (i = 0U; i < scene->path_count; ++i) {
        scene->paths[i].style.outline_enabled = enabled ?
                                                GP2D_TRUE : GP2D_FALSE;
        scene->paths[i].style.outline = color;
        scene->paths[i].style.outline_width = width;
    }
}
