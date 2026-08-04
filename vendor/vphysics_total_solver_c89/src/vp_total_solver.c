#include "vp_total_solver.h"
#include "vp_config.h"
#include <stddef.h>

static void vp_ts_install_callbacks(vpTotalSolver* s);

static void vp_ts_memzero(void* p, vp_u32 bytes)
{
    vp_u8* b = (vp_u8*)p;
    vp_u32 i;
    for (i = 0; i < bytes; ++i) b[i] = 0;
}

static vp_u32 vp_ts_align_size(vp_u32 value)
{
    vp_u32 a = (vp_u32)VP_ARENA_ALIGNMENT;
    vp_u32 mask = a - 1U;
    return (value + mask) & ~mask;
}

static vp_u8* vp_ts_align_ptr(vp_u8* p)
{
    size_t v = (size_t)p;
    size_t a = (size_t)VP_ARENA_ALIGNMENT;
    size_t mask = a - (size_t)1;
    v = (v + mask) & ~mask;
    return (vp_u8*)v;
}

static vpVec3 vp_ts_vec_mul(vpVec3 a, vpVec3 b)
{
    return vpVec3_make(vp_fx_mul(a.x, b.x), vp_fx_mul(a.y, b.y), vp_fx_mul(a.z, b.z));
}

static vpVec3 vp_ts_vec_lerp(vpVec3 a, vpVec3 b, vp_fx t)
{
    vpVec3 r;
    r.x = vp_fx_lerp(a.x, b.x, t);
    r.y = vp_fx_lerp(a.y, b.y, t);
    r.z = vp_fx_lerp(a.z, b.z, t);
    return r;
}

vpTransform vpTransformIdentity(void)
{
    vpTransform t;
    t.position = vpVec3_make(0,0,0);
    t.rotation = vpQuat_identity();
    t.scale = vpVec3_make(VP_FX_ONE, VP_FX_ONE, VP_FX_ONE);
    return t;
}

static void vp_ts_rotate(vpTotalSolver* s, vpVec3* out, const vpQuat* rotation, const vpVec3* vector)
{
    if ((s->math.capabilities & VP_TRANSFORM_MATH_ROTATE) && s->math.rotateVector)
        s->math.rotateVector(s->math.user, out, rotation, vector);
    else *out = vpQuat_rotate_vec3(*rotation, *vector);
}

static void vp_ts_qmul(vpTotalSolver* s, vpQuat* out, const vpQuat* a, const vpQuat* b)
{
    if ((s->math.capabilities & VP_TRANSFORM_MATH_QUAT_MUL) && s->math.quatMul)
        s->math.quatMul(s->math.user, out, a, b);
    else *out = vpQuat_mul(*a, *b);
}

static void vp_ts_qnormalize(vpTotalSolver* s, vpQuat* out, const vpQuat* q)
{
    if ((s->math.capabilities & VP_TRANSFORM_MATH_QUAT_NORMALIZE) && s->math.quatNormalize)
        s->math.quatNormalize(s->math.user, out, q);
    else *out = vpQuat_normalize(*q);
}

static void vp_ts_fallback_compose(vpTotalSolver* s, vpTransform* out, const vpTransform* parent, const vpTransform* local)
{
    vpVec3 scaled;
    vpVec3 rotated;
    vpQuat product;
    scaled = vp_ts_vec_mul(local->position, parent->scale);
    vp_ts_rotate(s, &rotated, &parent->rotation, &scaled);
    out->position = vpVec3_add(parent->position, rotated);
    vp_ts_qmul(s, &product, &parent->rotation, &local->rotation);
    vp_ts_qnormalize(s, &out->rotation, &product);
    out->scale = vp_ts_vec_mul(parent->scale, local->scale);
}

static void vp_ts_fallback_inverse(vpTotalSolver* s, vpTransform* out, const vpTransform* in)
{
    vpQuat norm;
    vpQuat invRot;
    vpVec3 neg;
    vp_ts_qnormalize(s, &norm, &in->rotation);
    invRot = vpQuat_conjugate(norm);
    out->rotation = invRot;
    out->scale.x = (in->scale.x != 0) ? vp_fx_div(VP_FX_ONE, in->scale.x) : 0;
    out->scale.y = (in->scale.y != 0) ? vp_fx_div(VP_FX_ONE, in->scale.y) : 0;
    out->scale.z = (in->scale.z != 0) ? vp_fx_div(VP_FX_ONE, in->scale.z) : 0;
    neg = vpVec3_make(-in->position.x, -in->position.y, -in->position.z);
    vp_ts_rotate(s, &neg, &invRot, &neg);
    out->position = vp_ts_vec_mul(neg, out->scale);
}

static void vp_ts_fallback_point(vpTotalSolver* s, vpVec3* out, const vpTransform* t, const vpVec3* point)
{
    vpVec3 p;
    p = vp_ts_vec_mul(*point, t->scale);
    vp_ts_rotate(s, &p, &t->rotation, &p);
    *out = vpVec3_add(t->position, p);
}

static void vp_ts_fallback_blend(vpTotalSolver* s, vpTransform* out, const vpTransform* a, const vpTransform* b, vp_fx weight)
{
    vpQuat qb;
    vpQuat q;
    vp_fx dot;
    weight = vp_fx_clamp(weight, 0, VP_FX_ONE);
    out->position = vp_ts_vec_lerp(a->position, b->position, weight);
    out->scale = vp_ts_vec_lerp(a->scale, b->scale, weight);
    qb = b->rotation;
    dot = vp_fx_mul(a->rotation.x, qb.x) + vp_fx_mul(a->rotation.y, qb.y) +
          vp_fx_mul(a->rotation.z, qb.z) + vp_fx_mul(a->rotation.w, qb.w);
    if (dot < 0) qb = vpQuat_neg(qb);
    q.x = vp_fx_lerp(a->rotation.x, qb.x, weight);
    q.y = vp_fx_lerp(a->rotation.y, qb.y, weight);
    q.z = vp_fx_lerp(a->rotation.z, qb.z, weight);
    q.w = vp_fx_lerp(a->rotation.w, qb.w, weight);
    vp_ts_qnormalize(s, &out->rotation, &q);
}

/* Fallback look-at rotates local +Z toward target. The provider may replace it
   with a roll-aware implementation using the supplied up vector. */
static void vp_ts_fallback_look_at(vpTotalSolver* s, vpQuat* out, const vpVec3* origin, const vpVec3* target, const vpVec3* up)
{
    vpVec3 from;
    vpVec3 dir;
    vpVec3 axis;
    vp_fx dot;
    vpQuat q;
    VP_UNUSED(up);
    from = vpVec3_make(0,0,VP_FX_ONE);
    dir = vpVec3_normalize(vpVec3_sub(*target, *origin));
    if (vpVec3_len2(dir) == 0) { *out = vpQuat_identity(); return; }
    dot = vpVec3_dot(from, dir);
    if (dot <= -VP_FX_ONE + (VP_FX_ONE >> 8)) {
        q.x = 0; q.y = VP_FX_ONE; q.z = 0; q.w = 0;
        *out = q;
        return;
    }
    axis = vpVec3_cross(from, dir);
    q.x = axis.x;
    q.y = axis.y;
    q.z = axis.z;
    q.w = VP_FX_ONE + dot;
    vp_ts_qnormalize(s, out, &q);
}

static void vp_ts_compose(vpTotalSolver* s, vpTransform* out, const vpTransform* parent, const vpTransform* local)
{
    if ((s->math.capabilities & VP_TRANSFORM_MATH_COMPOSE) && s->math.compose)
        s->math.compose(s->math.user, out, parent, local);
    else vp_ts_fallback_compose(s, out, parent, local);
}

static void vp_ts_inverse(vpTotalSolver* s, vpTransform* out, const vpTransform* in)
{
    if ((s->math.capabilities & VP_TRANSFORM_MATH_INVERSE) && s->math.inverse)
        s->math.inverse(s->math.user, out, in);
    else vp_ts_fallback_inverse(s, out, in);
}

static void vp_ts_point(vpTotalSolver* s, vpVec3* out, const vpTransform* t, const vpVec3* point)
{
    if ((s->math.capabilities & VP_TRANSFORM_MATH_POINT) && s->math.transformPoint)
        s->math.transformPoint(s->math.user, out, t, point);
    else vp_ts_fallback_point(s, out, t, point);
}

static void vp_ts_blend(vpTotalSolver* s, vpTransform* out, const vpTransform* a, const vpTransform* b, vp_fx weight)
{
    if ((s->math.capabilities & VP_TRANSFORM_MATH_BLEND) && s->math.blend)
        s->math.blend(s->math.user, out, a, b, weight);
    else vp_ts_fallback_blend(s, out, a, b, weight);
}

static void vp_ts_look_at(vpTotalSolver* s, vpQuat* out, const vpVec3* origin, const vpVec3* target, const vpVec3* up)
{
    if ((s->math.capabilities & VP_TRANSFORM_MATH_LOOK_AT) && s->math.lookAt)
        s->math.lookAt(s->math.user, out, origin, target, up);
    else vp_ts_fallback_look_at(s, out, origin, target, up);
}

void vpTotalSolverMathCompose(vpTotalSolver* s, vpTransform* out, const vpTransform* parent, const vpTransform* local)
{
    if (s && out && parent && local) vp_ts_compose(s, out, parent, local);
}

void vpTotalSolverMathInverse(vpTotalSolver* s, vpTransform* out, const vpTransform* value)
{
    if (s && out && value) vp_ts_inverse(s, out, value);
}

void vpTotalSolverMathTransformPoint(vpTotalSolver* s, vpVec3* out, const vpTransform* transform, const vpVec3* point)
{
    if (s && out && transform && point) vp_ts_point(s, out, transform, point);
}

void vpTotalSolverMathRotateVector(vpTotalSolver* s, vpVec3* out, const vpQuat* rotation, const vpVec3* vector)
{
    if (s && out && rotation && vector) vp_ts_rotate(s, out, rotation, vector);
}

void vpTotalSolverMathQuatMul(vpTotalSolver* s, vpQuat* out, const vpQuat* a, const vpQuat* b)
{
    if (s && out && a && b) vp_ts_qmul(s, out, a, b);
}

void vpTotalSolverMathQuatNormalize(vpTotalSolver* s, vpQuat* out, const vpQuat* value)
{
    if (s && out && value) vp_ts_qnormalize(s, out, value);
}

void vpTotalSolverMathBlend(vpTotalSolver* s, vpTransform* out, const vpTransform* a, const vpTransform* b, vp_fx weight)
{
    if (s && out && a && b) vp_ts_blend(s, out, a, b, weight);
}

void vpTotalSolverMathLookAt(vpTotalSolver* s, vpQuat* out, const vpVec3* origin, const vpVec3* target, const vpVec3* up)
{
    if (s && out && origin && target && up) vp_ts_look_at(s, out, origin, target, up);
}

vp_u32 vpTotalSolverMemSize(const vpTotalSolverDesc* desc)
{
    vp_u32 bytes = (vp_u32)VP_ARENA_ALIGNMENT - 1U;
#define VP_TS_ADD(type, count) do { bytes = vp_ts_align_size(bytes); bytes += (vp_u32)sizeof(type) * (vp_u32)(count); } while (0)
    VP_TS_ADD(vpTotalSolver, 1);
    VP_TS_ADD(vpTransformNode, desc->maxTransformNodes);
    VP_TS_ADD(vpTransformConstraint, desc->maxTransformConstraints);
    VP_TS_ADD(vp_u16, desc->maxTransformConstraints);
    VP_TS_ADD(vp_u8, desc->maxTransformNodes);
#undef VP_TS_ADD
    return bytes;
}

vpTotalSolver* vpTotalSolverInit(void* mem, vp_u32 memBytes, vpWorld* world, const vpTotalSolverDesc* desc)
{
    vp_u8* p;
    vpTotalSolver* s;
    vp_u32 need;
    if (!mem || !world || !desc) return (vpTotalSolver*)0;
    need = vpTotalSolverMemSize(desc);
    if (memBytes < need) return (vpTotalSolver*)0;
    p = vp_ts_align_ptr((vp_u8*)mem);
    s = (vpTotalSolver*)p;
    p += sizeof(vpTotalSolver);
    vp_ts_memzero(s, (vp_u32)sizeof(vpTotalSolver));
    s->world = world;
    s->maxNodes = desc->maxTransformNodes;
    s->maxConstraints = desc->maxTransformConstraints;
    s->iterations = desc->transformIterations ? desc->transformIterations : 2;

    p = vp_ts_align_ptr(p);
    s->nodes = (vpTransformNode*)p;
    p += sizeof(vpTransformNode) * (vp_u32)s->maxNodes;
    p = vp_ts_align_ptr(p);
    s->constraints = (vpTransformConstraint*)p;
    p += sizeof(vpTransformConstraint) * (vp_u32)s->maxConstraints;
    p = vp_ts_align_ptr(p);
    s->constraintOrder = (vp_u16*)p;
    p += sizeof(vp_u16) * (vp_u32)s->maxConstraints;
    p = vp_ts_align_ptr(p);
    s->visitState = (vp_u8*)p;

    vp_ts_memzero(s->nodes, (vp_u32)sizeof(vpTransformNode) * (vp_u32)s->maxNodes);
    vp_ts_memzero(s->constraints, (vp_u32)sizeof(vpTransformConstraint) * (vp_u32)s->maxConstraints);
    vp_ts_memzero(s->constraintOrder, (vp_u32)sizeof(vp_u16) * (vp_u32)s->maxConstraints);
    vp_ts_memzero(s->visitState, (vp_u32)sizeof(vp_u8) * (vp_u32)s->maxNodes);
    s->orderDirty = 1;
    return s;
}

void vpTotalSolverSetMathProvider(vpTotalSolver* solver, const vpTransformMathProvider* provider)
{
    if (!solver) return;
    if (provider) solver->math = *provider;
    else vp_ts_memzero(&solver->math, (vp_u32)sizeof(solver->math));
}

void vpTotalSolverSetTransformProvider(vpTotalSolver* solver, const vpTransformProvider* provider)
{
    if (!solver) return;
    if (provider) solver->transforms = *provider;
    else vp_ts_memzero(&solver->transforms, (vp_u32)sizeof(solver->transforms));
}

void vpTotalSolverSetCollisionProvider(vpTotalSolver* solver, const vpCollisionProvider* provider)
{
    if (!solver) return;
    if (provider) solver->collisions = *provider;
    else vp_ts_memzero(&solver->collisions, (vp_u32)sizeof(solver->collisions));
    if (solver->attached) vp_ts_install_callbacks(solver);
}

void vpTotalSolverSetLegacyCallbacks(vpTotalSolver* solver, const vpCallbacks* callbacks)
{
    if (!solver) return;
    if (callbacks) {
        solver->legacyCallbacks = *callbacks;
        solver->legacyCallbacksEnabled = 1;
    } else {
        vp_ts_memzero(&solver->legacyCallbacks, (vp_u32)sizeof(solver->legacyCallbacks));
        solver->legacyCallbacksEnabled = 0;
    }
    if (solver->attached) vp_ts_install_callbacks(solver);
}

vpTransformNode* vpTransformNodeGet(vpTotalSolver* solver, vpTransformNodeId node)
{
    if (!solver || node == 0 || node > solver->maxNodes) return (vpTransformNode*)0;
    if (!solver->nodes[node - 1].used) return (vpTransformNode*)0;
    return &solver->nodes[node - 1];
}

vpTransformNodeId vpTransformNodeCreate(vpTotalSolver* solver, vp_u32 externalId)
{
    vp_u16 i;
    vpTransformNode* n;
    if (!solver) return 0;
    for (i = 0; i < solver->maxNodes; ++i) {
        if (!solver->nodes[i].used) {
            n = &solver->nodes[i];
            vp_ts_memzero(n, (vp_u32)sizeof(*n));
            n->used = 1;
            n->flags = VP_TRANSFORM_NODE_ENABLED;
            if (externalId != 0) n->flags |= VP_TRANSFORM_NODE_READ_EXTERNAL | VP_TRANSFORM_NODE_WRITE_EXTERNAL;
            n->authority = (vp_u8)(externalId ? VP_TRANSFORM_AUTH_EXTERNAL : VP_TRANSFORM_AUTH_MANUAL);
            n->externalId = externalId;
            n->blendWeight = VP_FX_HALF;
            n->local = vpTransformIdentity();
            n->world = n->local;
            n->externalWorld = n->world;
            solver->nodeCount += 1;
            return (vpTransformNodeId)(i + 1);
        }
    }
    return 0;
}

void vpTransformNodeDestroy(vpTotalSolver* solver, vpTransformNodeId node)
{
    vpTransformNode* n;
    vp_u16 i;
    if (!solver) return;
    n = vpTransformNodeGet(solver, node);
    if (!n) return;
    for (i = 0; i < solver->maxNodes; ++i) {
        if (solver->nodes[i].used && solver->nodes[i].parent == node) solver->nodes[i].parent = 0;
    }
    for (i = 0; i < solver->maxConstraints; ++i) {
        if (solver->constraints[i].used &&
            (solver->constraints[i].target == node || solver->constraints[i].source == node)) {
            solver->constraints[i].used = 0;
            if (solver->constraintCount) solver->constraintCount -= 1;
        }
    }
    vp_ts_memzero(n, (vp_u32)sizeof(*n));
    if (solver->nodeCount) solver->nodeCount -= 1;
    solver->orderDirty = 1;
}

void vpTransformNodeSetParent(vpTotalSolver* solver, vpTransformNodeId node, vpTransformNodeId parent)
{
    vpTransformNode* n = vpTransformNodeGet(solver, node);
    if (!n) return;
    if (parent != 0 && !vpTransformNodeGet(solver, parent)) return;
    if (parent == node) { solver->cycleCount += 1; return; }
    n->parent = parent;
    n->dirty = 1;
}

void vpTransformNodeBindBody(vpTotalSolver* solver, vpTransformNodeId node, vpBodyId body, vpTransformAuthority authority)
{
    vpTransformNode* n = vpTransformNodeGet(solver, node);
    if (!n || !solver->world) return;
    if (body != 0 && !vpWorldGetBody(solver->world, body)) return;
    n->body = body;
    n->authority = (vp_u8)authority;
}

void vpTransformNodeSetAuthority(vpTotalSolver* solver, vpTransformNodeId node, vpTransformAuthority authority, vp_fx blendWeight)
{
    vpTransformNode* n = vpTransformNodeGet(solver, node);
    if (!n) return;
    n->authority = (vp_u8)authority;
    n->blendWeight = vp_fx_clamp(blendWeight, 0, VP_FX_ONE);
}

static void vp_ts_world_to_local(vpTotalSolver* solver, vpTransformNode* n)
{
    vpTransformNode* parent;
    vpTransform inv;
    if (!n || n->parent == 0) { n->local = n->world; return; }
    parent = vpTransformNodeGet(solver, n->parent);
    if (!parent) { n->parent = 0; n->local = n->world; return; }
    vp_ts_inverse(solver, &inv, &parent->world);
    vp_ts_compose(solver, &n->local, &inv, &n->world);
}

void vpTransformNodeSetLocal(vpTotalSolver* solver, vpTransformNodeId node, const vpTransform* value)
{
    vpTransformNode* n = vpTransformNodeGet(solver, node);
    if (!n || !value) return;
    n->local = *value;
    n->dirty = 1;
}

void vpTransformNodeSetWorld(vpTotalSolver* solver, vpTransformNodeId node, const vpTransform* value)
{
    vpTransformNode* n = vpTransformNodeGet(solver, node);
    if (!n || !value) return;
    n->world = *value;
    vp_ts_world_to_local(solver, n);
    n->dirty = 0;
}

vpTransformConstraint* vpTransformConstraintGet(vpTotalSolver* solver, vpTransformConstraintId id)
{
    if (!solver || id == 0 || id > solver->maxConstraints) return (vpTransformConstraint*)0;
    if (!solver->constraints[id - 1].used) return (vpTransformConstraint*)0;
    return &solver->constraints[id - 1];
}

vpTransformConstraintId vpTransformConstraintCreate(vpTotalSolver* solver,
    vpTransformConstraintType type, vpTransformPhase phase,
    vpTransformNodeId target, vpTransformNodeId source, vp_u8 priority)
{
    vp_u16 i;
    vpTransformConstraint* c;
    if (!solver || !vpTransformNodeGet(solver, target)) return 0;
    if (source && !vpTransformNodeGet(solver, source)) return 0;
    for (i = 0; i < solver->maxConstraints; ++i) {
        if (!solver->constraints[i].used) {
            c = &solver->constraints[i];
            vp_ts_memzero(c, (vp_u32)sizeof(*c));
            c->used = 1;
            c->enabled = 1;
            c->type = (vp_u8)type;
            c->phase = (vp_u8)phase;
            c->priority = priority;
            c->target = target;
            c->source = source;
            c->weight = VP_FX_ONE;
            c->offset = vpTransformIdentity();
            c->minValue = vpVec3_make(VP_FX_MIN, VP_FX_MIN, VP_FX_MIN);
            c->maxValue = vpVec3_make(VP_FX_MAX, VP_FX_MAX, VP_FX_MAX);
            solver->constraintCount += 1;
            solver->orderDirty = 1;
            return (vpTransformConstraintId)(i + 1);
        }
    }
    return 0;
}

void vpTransformConstraintDestroy(vpTotalSolver* solver, vpTransformConstraintId id)
{
    vpTransformConstraint* c = vpTransformConstraintGet(solver, id);
    if (!c) return;
    vp_ts_memzero(c, (vp_u32)sizeof(*c));
    if (solver->constraintCount) solver->constraintCount -= 1;
    solver->orderDirty = 1;
}

void vpTransformConstraintSetWeight(vpTotalSolver* solver, vpTransformConstraintId id, vp_fx weight)
{
    vpTransformConstraint* c = vpTransformConstraintGet(solver, id);
    if (c) c->weight = vp_fx_clamp(weight, 0, VP_FX_ONE);
}

void vpTransformConstraintSetOffset(vpTotalSolver* solver, vpTransformConstraintId id, const vpTransform* offset)
{
    vpTransformConstraint* c = vpTransformConstraintGet(solver, id);
    if (c && offset) c->offset = *offset;
}

void vpTransformConstraintSetLimits(vpTotalSolver* solver, vpTransformConstraintId id, vpVec3 minValue, vpVec3 maxValue)
{
    vpTransformConstraint* c = vpTransformConstraintGet(solver, id);
    if (!c) return;
    c->minValue = minValue;
    c->maxValue = maxValue;
}

void vpTransformConstraintSetDistance(vpTotalSolver* solver, vpTransformConstraintId id, vp_fx distance)
{
    vpTransformConstraint* c = vpTransformConstraintGet(solver, id);
    if (c) c->distance = distance;
}

static void vp_ts_resolve_node(vpTotalSolver* s, vpTransformNodeId id)
{
    vpTransformNode* n;
    vpTransformNode* p;
    vp_u16 idx;
    if (id == 0 || id > s->maxNodes) return;
    idx = (vp_u16)(id - 1);
    n = &s->nodes[idx];
    if (!n->used) return;
    if (s->visitState[idx] == 2) return;
    if (s->visitState[idx] == 1) {
        s->cycleCount += 1;
        n->world = n->local;
        s->visitState[idx] = 2;
        return;
    }
    s->visitState[idx] = 1;
    if (n->parent) {
        vp_ts_resolve_node(s, n->parent);
        p = vpTransformNodeGet(s, n->parent);
        if (p) vp_ts_compose(s, &n->world, &p->world, &n->local);
        else { n->parent = 0; n->world = n->local; }
    } else n->world = n->local;
    n->dirty = 0;
    s->visitState[idx] = 2;
}

static void vp_ts_resolve_hierarchy(vpTotalSolver* s)
{
    vp_u16 i;
    vp_ts_memzero(s->visitState, (vp_u32)sizeof(vp_u8) * (vp_u32)s->maxNodes);
    for (i = 0; i < s->maxNodes; ++i) {
        if (s->nodes[i].used) vp_ts_resolve_node(s, (vpTransformNodeId)(i + 1));
    }
}

static vpTransformNodeId vp_ts_find_external_node(vpTotalSolver* s, vp_u32 externalId)
{
    vp_u16 i;
    if (!externalId) return 0;
    for (i = 0; i < s->maxNodes; ++i) {
        if (s->nodes[i].used && s->nodes[i].externalId == externalId)
            return (vpTransformNodeId)(i + 1);
    }
    return 0;
}

void vpTotalSolverPullTransforms(vpTotalSolver* s)
{
    vp_u16 i;
    vpTransformNode* n;
    int ok;
    if (!s) return;

    /* Parent contracts may come from the external scene graph. */
    if (s->transforms.getParent) {
        for (i = 0; i < s->maxNodes; ++i) {
            vp_u32 parentExternal = 0;
            vpTransformNodeId parentNode;
            n = &s->nodes[i];
            if (!n->used || !n->externalId) continue;
            if (s->transforms.getParent(s->transforms.user, n->externalId, &parentExternal)) {
                parentNode = vp_ts_find_external_node(s, parentExternal);
                if (parentNode != (vpTransformNodeId)(i + 1)) n->parent = parentNode;
                else s->cycleCount += 1;
            }
        }
    }

    /* dirty=1 means local was supplied; dirty=2 means world was supplied. */
    for (i = 0; i < s->maxNodes; ++i) {
        n = &s->nodes[i];
        if (!n->used || !(n->flags & VP_TRANSFORM_NODE_ENABLED)) continue;
        if (!n->externalId || !(n->flags & VP_TRANSFORM_NODE_READ_EXTERNAL)) continue;
        ok = 0;
        if (s->transforms.readLocal) {
            ok = s->transforms.readLocal(s->transforms.user, n->externalId, &n->local);
            if (ok) n->dirty = 1;
        }
        if (!ok && s->transforms.readWorld) {
            ok = s->transforms.readWorld(s->transforms.user, n->externalId, &n->externalWorld);
            if (ok) {
                n->world = n->externalWorld;
                n->dirty = 2;
            }
        }
        if (!ok) s->providerErrorCount += 1;
    }

    /* Convert supplied world poses to locals using the current supplied parent pose. */
    for (i = 0; i < s->maxNodes; ++i) {
        vpTransformNode* parent;
        vpTransform inv;
        n = &s->nodes[i];
        if (!n->used || n->dirty != 2) continue;
        parent = vpTransformNodeGet(s, n->parent);
        if (parent) {
            const vpTransform* parentWorld = (parent->dirty == 2) ? &parent->externalWorld : &parent->world;
            vp_ts_inverse(s, &inv, parentWorld);
            vp_ts_compose(s, &n->local, &inv, &n->externalWorld);
        } else n->local = n->externalWorld;
    }

    vp_ts_resolve_hierarchy(s);
    for (i = 0; i < s->maxNodes; ++i) {
        n = &s->nodes[i];
        if (n->used && n->externalId && (n->flags & VP_TRANSFORM_NODE_READ_EXTERNAL)) {
            n->externalWorld = n->world;
            n->dirty = 0;
        }
    }
}

static void vp_ts_sort_constraints(vpTotalSolver* s)
{
    vp_u16 i, j, n;
    n = 0;
    for (i = 0; i < s->maxConstraints; ++i) {
        if (s->constraints[i].used) s->constraintOrder[n++] = i;
    }
    for (i = 1; i < n; ++i) {
        vp_u16 key = s->constraintOrder[i];
        vp_u8 priority = s->constraints[key].priority;
        j = i;
        while (j > 0 && s->constraints[s->constraintOrder[j - 1]].priority > priority) {
            s->constraintOrder[j] = s->constraintOrder[j - 1];
            --j;
        }
        s->constraintOrder[j] = key;
    }
    s->constraintCount = n;
    s->orderDirty = 0;
}

static void vp_ts_apply_constraint(vpTotalSolver* s, vpTransformConstraint* c)
{
    vpTransformNode* t;
    vpTransformNode* src;
    vpTransform desired;
    vpTransform blended;
    vpVec3 targetPos;
    vpVec3 d;
    vp_fx len;
    vpVec3 up;
    if (!c->enabled) return;
    t = vpTransformNodeGet(s, c->target);
    src = c->source ? vpTransformNodeGet(s, c->source) : (vpTransformNode*)0;
    if (!t) return;
    desired = t->world;

    if (c->type == VP_TRANSFORM_CONSTRAINT_PARENT && src) {
        vp_ts_compose(s, &desired, &src->world, &c->offset);
    } else if (c->type == VP_TRANSFORM_CONSTRAINT_COPY_POSITION && src) {
        desired.position = vpVec3_add(src->world.position, c->offset.position);
    } else if (c->type == VP_TRANSFORM_CONSTRAINT_COPY_ROTATION && src) {
        vpQuat product;
        vp_ts_qmul(s, &product, &src->world.rotation, &c->offset.rotation);
        vp_ts_qnormalize(s, &desired.rotation, &product);
    } else if (c->type == VP_TRANSFORM_CONSTRAINT_COPY_SCALE && src) {
        desired.scale = vp_ts_vec_mul(src->world.scale, c->offset.scale);
    } else if (c->type == VP_TRANSFORM_CONSTRAINT_LOOK_AT && src) {
        targetPos = vpVec3_add(src->world.position, c->offset.position);
        up = vpVec3_make(0,VP_FX_ONE,0);
        vp_ts_look_at(s, &desired.rotation, &t->world.position, &targetPos, &up);
    } else if (c->type == VP_TRANSFORM_CONSTRAINT_DISTANCE && src) {
        d = vpVec3_sub(t->world.position, src->world.position);
        len = vpVec3_len(d);
        if (len > 0) {
            vp_fx wanted = c->distance;
            if (wanted < 0) wanted = 0;
            desired.position = vpVec3_add(src->world.position,
                vpVec3_scale(d, vp_fx_div(wanted, len)));
        }
    } else if (c->type == VP_TRANSFORM_CONSTRAINT_POSITION_LIMIT) {
        vpVec3 base = src ? src->world.position : vpVec3_make(0,0,0);
        vpVec3 rel = vpVec3_sub(t->world.position, base);
        rel.x = vp_fx_clamp(rel.x, c->minValue.x, c->maxValue.x);
        rel.y = vp_fx_clamp(rel.y, c->minValue.y, c->maxValue.y);
        rel.z = vp_fx_clamp(rel.z, c->minValue.z, c->maxValue.z);
        desired.position = vpVec3_add(base, rel);
    } else if (c->type == VP_TRANSFORM_CONSTRAINT_FOLLOW && src) {
        vp_ts_compose(s, &desired, &src->world, &c->offset);
    } else return;

    vp_ts_blend(s, &blended, &t->world, &desired, c->weight);
    t->world = blended;
    vp_ts_world_to_local(s, t);
}

static void vp_ts_solve_constraints(vpTotalSolver* s, vp_u8 phase)
{
    vp_u16 iter, i;
    vpTransformConstraint* c;
    if (s->orderDirty) vp_ts_sort_constraints(s);
    for (iter = 0; iter < s->iterations; ++iter) {
        for (i = 0; i < s->constraintCount; ++i) {
            c = &s->constraints[s->constraintOrder[i]];
            if (c->used && (c->phase & phase)) vp_ts_apply_constraint(s, c);
        }
    }
}

static void vp_ts_drive_bodies(vpTotalSolver* s)
{
    vp_u16 i;
    vpTransformNode* n;
    vpBody* b;
    for (i = 0; i < s->maxNodes; ++i) {
        n = &s->nodes[i];
        if (!n->used || !n->body) continue;
        b = vpWorldGetBody(s->world, (vpBodyId)n->body);
        if (!b || !b->used) continue;
        if (n->authority == VP_TRANSFORM_AUTH_EXTERNAL || n->authority == VP_TRANSFORM_AUTH_MANUAL) {
            if (b->isKinematic || (n->flags & VP_TRANSFORM_NODE_DRIVE_DYNAMIC))
                vpBodySetPose(s->world, (vpBodyId)n->body, n->world.position, n->world.rotation);
        } else if (n->authority == VP_TRANSFORM_AUTH_BLEND && b->isKinematic) {
            vpBodySetPose(s->world, (vpBodyId)n->body, n->world.position, n->world.rotation);
        }
    }
}

void vpTotalSolverSolvePrePhysics(vpTotalSolver* s, vp_fx dt)
{
    if (!s) return;
    VP_UNUSED(dt);
    vp_ts_resolve_hierarchy(s);
    vp_ts_solve_constraints(s, VP_TRANSFORM_PHASE_PRE_PHYSICS);
    vp_ts_drive_bodies(s);
}

static void vp_ts_read_bodies(vpTotalSolver* s)
{
    vp_u16 i;
    vpTransformNode* n;
    vpBody* b;
    vpTransform bodyWorld;
    for (i = 0; i < s->maxNodes; ++i) {
        n = &s->nodes[i];
        if (!n->used || !n->body) continue;
        b = vpWorldGetBody(s->world, (vpBodyId)n->body);
        if (!b || !b->used) continue;
        bodyWorld = n->world;
        bodyWorld.position = b->pos;
        bodyWorld.rotation = b->rot;
        if (n->authority == VP_TRANSFORM_AUTH_PHYSICS) {
            n->world = bodyWorld;
            vp_ts_world_to_local(s, n);
        } else if (n->authority == VP_TRANSFORM_AUTH_BLEND) {
            vp_ts_blend(s, &n->world, &n->externalWorld, &bodyWorld, n->blendWeight);
            vp_ts_world_to_local(s, n);
        }
    }
}

void vpTotalSolverSolvePostPhysics(vpTotalSolver* s, vp_fx dt)
{
    if (!s) return;
    VP_UNUSED(dt);
    vp_ts_read_bodies(s);
    vp_ts_resolve_hierarchy(s);
    vp_ts_solve_constraints(s, VP_TRANSFORM_PHASE_POST_PHYSICS);
}

void vpTotalSolverPushTransforms(vpTotalSolver* s)
{
    vp_u16 i;
    vpTransformNode* n;
    int ok;
    if (!s) return;
    for (i = 0; i < s->maxNodes; ++i) {
        n = &s->nodes[i];
        if (!n->used || !(n->flags & VP_TRANSFORM_NODE_ENABLED)) continue;
        if (!n->externalId || !(n->flags & VP_TRANSFORM_NODE_WRITE_EXTERNAL)) continue;
        ok = 0;
        if (s->transforms.writeWorld) ok = s->transforms.writeWorld(s->transforms.user, n->externalId, &n->world);
        if (!ok && s->transforms.writeLocal) ok = s->transforms.writeLocal(s->transforms.user, n->externalId, &n->local);
        if (!ok) s->providerErrorCount += 1;
    }
}

static void vp_ts_pre_step(void* user, vpWorld* world, vp_fx dt)
{
    vpTotalSolver* s = (vpTotalSolver*)user;
    if (!s || s->inCallback) return;
    s->inCallback = 1;
    vpTotalSolverPullTransforms(s);
    vpTotalSolverSolvePrePhysics(s, dt);
    if (s->legacyCallbacksEnabled && s->legacyCallbacks.preStep)
        s->legacyCallbacks.preStep(s->legacyCallbacks.user, world, dt);
    if (s->collisions.beginStep) s->collisions.beginStep(s->collisions.user, s, world, dt);
    if (s->collisions.generateContacts) s->collisions.generateContacts(s->collisions.user, s, world, dt);
    s->inCallback = 0;
}

static void vp_ts_post_step(void* user, vpWorld* world, vp_fx dt)
{
    vpTotalSolver* s = (vpTotalSolver*)user;
    if (!s || s->inCallback) return;
    s->inCallback = 1;
    vpTotalSolverSolvePostPhysics(s, dt);
    vpTotalSolverPushTransforms(s);
    if (s->collisions.endStep) s->collisions.endStep(s->collisions.user, s, world, dt);
    if (s->legacyCallbacksEnabled && s->legacyCallbacks.postStep)
        s->legacyCallbacks.postStep(s->legacyCallbacks.user, world, dt);
    s->inCallback = 0;
}

static vp_fx vp_ts_sweep(void* user, vp_u16 bodyId,
    vpVec3 fromPos, vpQuat fromRot, vpVec3 toPos, vpQuat toRot,
    vp_u16* hitBody, vpVec3* hitPoint, vpVec3* hitNormal)
{
    vpTotalSolver* s = (vpTotalSolver*)user;
    if (s && s->collisions.bodySweepTOI)
        return s->collisions.bodySweepTOI(s->collisions.user, bodyId,
            fromPos, fromRot, toPos, toRot, hitBody, hitPoint, hitNormal);
    if (s && s->legacyCallbacksEnabled && s->legacyCallbacks.bodySweepTOI)
        return s->legacyCallbacks.bodySweepTOI(s->legacyCallbacks.user, bodyId,
            fromPos, fromRot, toPos, toRot, hitBody, hitPoint, hitNormal);
    return VP_FX_ONE;
}

static void vp_ts_install_callbacks(vpTotalSolver* s)
{
    vpCallbacks cb;
    if (!s || !s->world) return;
    vp_ts_memzero(&cb, (vp_u32)sizeof(cb));
    cb.user = s;
    cb.preStep = vp_ts_pre_step;
    cb.postStep = vp_ts_post_step;
    if (s->collisions.bodySweepTOI ||
        (s->legacyCallbacksEnabled && s->legacyCallbacks.bodySweepTOI))
        cb.bodySweepTOI = vp_ts_sweep;
    else cb.bodySweepTOI = 0;
    vpWorldSetCallbacks(s->world, &cb);
}

int vpTotalSolverAttach(vpTotalSolver* s)
{
    if (!s || !s->world) return 0;
    if (s->attached) return 1;
    if (!s->legacyCallbacksEnabled && s->world->callbacksEnabled) {
        s->legacyCallbacks = s->world->callbacks;
        s->legacyCallbacksEnabled = 1;
    }
    s->attached = 1;
    vp_ts_install_callbacks(s);
    return 1;
}

void vpTotalSolverDetach(vpTotalSolver* s)
{
    if (!s || !s->attached || !s->world) return;
    if (s->legacyCallbacksEnabled) vpWorldSetCallbacks(s->world, &s->legacyCallbacks);
    else vpWorldSetCallbacks(s->world, (const vpCallbacks*)0);
    s->attached = 0;
}

void vpTotalSolverStep(vpTotalSolver* s, vp_fx dt)
{
    if (!s || !s->world) return;
    if (!s->attached && !vpTotalSolverAttach(s)) return;
    vpWorldStep(s->world, dt);
}

void vpTotalSolverStepCCD(vpTotalSolver* s, vp_fx dt, vp_u16 maxSubSteps)
{
    if (!s || !s->world) return;
    if (!s->attached && !vpTotalSolverAttach(s)) return;
    vpWorldStepCCD(s->world, dt, maxSubSteps);
}
