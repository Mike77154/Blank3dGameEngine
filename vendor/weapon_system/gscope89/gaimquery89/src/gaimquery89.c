#include "../include/gaimquery89.h"

static g89_fx gaq89_fx_mul(g89_fx a, g89_fx b)
{
    return (g89_fx)((a / 256L) * (b / 256L));
}

static void gaq89_zero_vec(g89_vec3 *v)
{
    v->x = 0;
    v->y = 0;
    v->z = 0;
}

static g89_vec3 gaq89_endpoint(const g89_vec3 *from,
                               const g89_vec3 *dir,
                               g89_fx distance)
{
    g89_vec3 out;
    out.x = from->x + gaq89_fx_mul(dir->x, distance);
    out.y = from->y + gaq89_fx_mul(dir->y, distance);
    out.z = from->z + gaq89_fx_mul(dir->z, distance);
    return out;
}

void gaq89_init(gaq89_ctx *ctx, gaq89_raycast_cb raycast_cb, void *user)
{
    if (!ctx) return;
    ctx->raycast_cb = raycast_cb;
    ctx->user = user;
    gaq89_clear(ctx);
}

void gaq89_set_callback(gaq89_ctx *ctx,
                        gaq89_raycast_cb raycast_cb,
                        void *user)
{
    if (!ctx) return;
    ctx->raycast_cb = raycast_cb;
    ctx->user = user;
}

void gaq89_clear(gaq89_ctx *ctx)
{
    if (!ctx) return;
    gaq89_zero_vec(&ctx->ray_from);
    gaq89_zero_vec(&ctx->ray_dir);
    gaq89_zero_vec(&ctx->miss_endpoint);
    ctx->max_distance = 0;
    ctx->last_hit.valid = 0;
    ctx->last_hit.target_id = -1;
    ctx->last_hit.target_kind = GAQ89_TARGET_UNKNOWN;
    ctx->last_hit.target_part = -1;
    ctx->last_hit.material_id = -1;
    ctx->last_hit.flags = 0;
    ctx->last_hit.distance = 0;
    gaq89_zero_vec(&ctx->last_hit.position);
    gaq89_zero_vec(&ctx->last_hit.normal);
}

int gaq89_query_ray(gaq89_ctx *ctx,
                    const g89_vec3 *from,
                    const g89_vec3 *dir,
                    g89_fx max_distance)
{
    int hit;
    gaq89_hit result;

    if (!ctx || !from || !dir) return 0;
    ctx->ray_from = *from;
    ctx->ray_dir = *dir;
    ctx->max_distance = max_distance;
    ctx->miss_endpoint = gaq89_endpoint(from, dir, max_distance);

    result.valid = 0;
    result.target_id = -1;
    result.target_kind = GAQ89_TARGET_UNKNOWN;
    result.target_part = -1;
    result.material_id = -1;
    result.flags = 0;
    result.distance = 0;
    gaq89_zero_vec(&result.position);
    gaq89_zero_vec(&result.normal);

    if (!ctx->raycast_cb) {
        ctx->last_hit = result;
        return 0;
    }

    hit = ctx->raycast_cb(ctx->user, from, dir, max_distance, &result);
    if (hit) {
        result.valid = 1;
        if (result.distance < 0) result.distance = 0;
        if (result.distance > max_distance) result.distance = max_distance;
        ctx->last_hit = result;
        return 1;
    }

    ctx->last_hit = result;
    return 0;
}

int gaq89_query_center(gaq89_ctx *ctx,
                       const g89_camera *camera,
                       g89_fx max_distance)
{
    if (!ctx || !camera) return 0;
    return gaq89_query_ray(ctx,
                           &camera->pos,
                           &camera->forward,
                           max_distance);
}

const gaq89_hit *gaq89_get_last_hit(const gaq89_ctx *ctx)
{
    if (!ctx) return 0;
    return &ctx->last_hit;
}
