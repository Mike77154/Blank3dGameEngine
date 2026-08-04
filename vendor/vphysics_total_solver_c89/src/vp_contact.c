#include "vp_world.h"
#include "vp_config.h"

/* simple integer hash */
static vp_u32 vp_hash_u32(vp_u32 x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

static vpContactCacheEntry* vp_find_contact_cache(vpWorld* w, vp_u32 key)
{
    vp_u32 mask = w->contactCacheMask;
    vp_u32 idx = vp_hash_u32(key) & mask;
    vp_u32 start = idx;
    for (;;) {
        vpContactCacheEntry* e = &w->contactCache[idx];
        if (!e->used) return e;
        if (e->key == key) return e;
        idx = (idx + 1) & mask;
        if (idx == start) return e;
    }
}

static vpPairCacheEntry* vp_find_pair_cache(vpWorld* w, vp_u32 pairKey)
{
    vp_u32 mask = w->pairCacheMask;
    vp_u32 idx = vp_hash_u32(pairKey) & mask;
    vp_u32 start = idx;
    vpPairCacheEntry* oldest = &w->pairCache[idx];
    for (;;) {
        vpPairCacheEntry* e = &w->pairCache[idx];
        if (!e->used) return e;
        if (e->pairKey == pairKey) return e;
        if (e->lastSeenStep < oldest->lastSeenStep) oldest = e;
        idx = (idx + 1) & mask;
        if (idx == start) return oldest;
    }
}

static void vp_push_contact_event(vpWorld* w, vpEventType type, vp_u16 a, vp_u16 b, vp_u32 pairKey)
{
    if (!w->events) return;
    if (w->eventCount >= (vp_u16)VP_EVENT_QUEUE_SIZE) return;
    {
        vpEvent* e = &w->events[w->eventCount++];
        e->type = (vp_u8)type;
        e->_pad = 0;
        e->bodyA = a;
        e->bodyB = b;
        e->jointId = 0;
        e->pairKey = pairKey;
    }
}

void vpContactsBegin(vpWorld* w)
{
    w->contactRawCount = 0;
}

/* Internal add (to raw list) */
static void vp_add_raw(vpWorld* w,
    vp_u32 key,
    vpBodyId a, vpBodyId b,
    vpVec3 p, vpVec3 n,
    vp_fx penetration,
    vp_fx friction,
    vp_fx restitution,
    vp_u8 flags)
{
    vpContact* c;
    vp_u16 A = (vp_u16)a;
    vp_u16 B = (vp_u16)b;

    if (w->contactRawCount >= w->maxContacts) return;

    /* default materials */
    if (friction == 0) friction = w->defaultFriction;
    if (restitution == 0) restitution = w->defaultRestitution;

    /* canonicalize ordering for stable pair keys */
    if (A > B) {
        vp_u16 tmp = A; A = B; B = tmp;
        n.x = -n.x; n.y = -n.y; n.z = -n.z;
    }

    c = &w->contactsRaw[w->contactRawCount++];
    c->key = key; /* optional user-provided stable key (0 == auto) */
    c->bodyA = A;
    c->bodyB = B;
    c->flags = flags;
    c->_pad = 0;

    c->p = p;
    c->n = n;
    c->penetration = penetration;
    c->friction = friction;
    c->restitution = restitution;
    c->restitutionTarget = 0;

    c->lambdaN = 0;
    c->lambdaT1 = 0;
    c->lambdaT2 = 0;
#if VP_ENABLE_SPLIT_IMPULSE
    c->lambdaBias = 0;
#endif
}

void vpContactsAdd(vpWorld* w,
    vpBodyId a, vpBodyId b,
    vpVec3 p, vpVec3 n,
    vp_fx penetration,
    vp_fx friction,
    vp_fx restitution)
{
    vp_add_raw(w, 0, a, b, p, n, penetration, friction, restitution, (vp_u8)VP_CONTACT_FLAG_NONE);
}

void vpContactsAddKeyed(vpWorld* w,
    vp_u32 key,
    vpBodyId a, vpBodyId b,
    vpVec3 p, vpVec3 n,
    vp_fx penetration,
    vp_fx friction,
    vp_fx restitution)
{
    vp_add_raw(w, key, a, b, p, n, penetration, friction, restitution, (vp_u8)VP_CONTACT_FLAG_NONE);
}

void vpContactsAddEx(vpWorld* w,
    vpBodyId a, vpBodyId b,
    vpVec3 p, vpVec3 n,
    vp_fx penetration,
    vp_fx friction,
    vp_fx restitution,
    vp_u8 flags)
{
    vp_add_raw(w, 0, a, b, p, n, penetration, friction, restitution, flags);
}

void vpContactsAddKeyedEx(vpWorld* w,
    vp_u32 key,
    vpBodyId a, vpBodyId b,
    vpVec3 p, vpVec3 n,
    vp_fx penetration,
    vp_fx friction,
    vp_fx restitution,
    vp_u8 flags)
{
    vp_add_raw(w, key, a, b, p, n, penetration, friction, restitution, flags);
}

/* --- manifold reduction helpers --- */

static vp_fx vp_dist2_tangent(vpVec3 p, vpVec3 ref, vpVec3 t1, vpVec3 t2)
{
    vpVec3 d = vpVec3_sub(p, ref);
    vp_fx u = vpVec3_dot(d, t1);
    vp_fx v = vpVec3_dot(d, t2);
    vp_fx uu = vp_fx_mul(u, u);
    vp_fx vv = vp_fx_mul(v, v);
    return uu + vv;
}

/* 2D cross magnitude on tangent plane (using t1,t2 axes) */
static vp_fx vp_area2_tangent(vpVec3 p0, vpVec3 p1, vpVec3 p2, vpVec3 t1, vpVec3 t2)
{
    vpVec3 a = vpVec3_sub(p1, p0);
    vpVec3 b = vpVec3_sub(p2, p0);
    vp_fx ax = vpVec3_dot(a, t1);
    vp_fx ay = vpVec3_dot(a, t2);
    vp_fx bx = vpVec3_dot(b, t1);
    vp_fx by = vpVec3_dot(b, t2);
    vp_fx cross = vp_fx_mul(ax, by) - vp_fx_mul(ay, bx);
    return vp_fx_abs(cross);
}

static vp_u32 vp_pair_key(vp_u16 a, vp_u16 b)
{
    return ((vp_u32)a << 16) | (vp_u32)b;
}

static vp_fx vp_local_match_dist2(vpVec3 a, vpVec3 b)
{
    vpVec3 d = vpVec3_sub(a, b);
    return vpVec3_len2(d);
}

/*
  Select up to VP_MANIFOLD_MAX_POINTS indices from candIdx[] (size nCand).
  Returns count, writes output indices in outSel[].
*/
static vp_u16 vp_select_manifold_points(vpWorld* w, const vp_u16* candIdx, vp_u16 nCand, vpVec3 normal, vp_u16* outSel)
{
    vp_u16 i, best;
    vp_fx bestPen;
    vpVec3 t1, t2;
    vp_u16 selCount = 0;

    VP_UNUSED(w);

    if (nCand == 0) return 0;

    vpVec3_tangent_basis(normal, &t1, &t2);

    /* 1) deepest penetration */
    best = candIdx[0];
    bestPen = w->contactsRaw[best].penetration;
    for (i = 1; i < nCand; ++i) {
        vp_u16 idx = candIdx[i];
        vp_fx pen = w->contactsRaw[idx].penetration;
        if (pen > bestPen) { bestPen = pen; best = idx; }
    }
    outSel[selCount++] = best;
    if (selCount >= (vp_u16)VP_MANIFOLD_MAX_POINTS || nCand == 1) return selCount;

    /* 2) farthest from p0 in tangent plane */
    {
        vpVec3 p0 = w->contactsRaw[best].p;
        vp_fx bestD2 = -1;
        vp_u16 best2 = best;
        for (i = 0; i < nCand; ++i) {
            vp_u16 idx = candIdx[i];
            vp_fx d2;
            if (idx == best) continue;
            d2 = vp_dist2_tangent(w->contactsRaw[idx].p, p0, t1, t2);
            if (d2 > bestD2) { bestD2 = d2; best2 = idx; }
        }
        outSel[selCount++] = best2;
    }
    if (selCount >= (vp_u16)VP_MANIFOLD_MAX_POINTS || nCand == 2) return selCount;

    /* 3) maximize triangle area */
    {
        vpVec3 p0 = w->contactsRaw[outSel[0]].p;
        vpVec3 p1 = w->contactsRaw[outSel[1]].p;
        vp_fx bestA2 = -1;
        vp_u16 best3 = outSel[1];
        for (i = 0; i < nCand; ++i) {
            vp_u16 idx = candIdx[i];
            vp_fx a2;
            if (idx == outSel[0] || idx == outSel[1]) continue;
            a2 = vp_area2_tangent(p0, p1, w->contactsRaw[idx].p, t1, t2);
            if (a2 > bestA2) { bestA2 = a2; best3 = idx; }
        }
        outSel[selCount++] = best3;
    }
    if (selCount >= (vp_u16)VP_MANIFOLD_MAX_POINTS || nCand == 3) return selCount;

    /* 4) maximize minimum distance to existing points (tangent plane) */
    {
        vp_fx bestMinD2 = -1;
        vp_u16 best4 = outSel[2];
        vpVec3 pA = w->contactsRaw[outSel[0]].p;
        vpVec3 pB = w->contactsRaw[outSel[1]].p;
        vpVec3 pC = w->contactsRaw[outSel[2]].p;
        for (i = 0; i < nCand; ++i) {
            vp_u16 idx = candIdx[i];
            vpVec3 p = w->contactsRaw[idx].p;
            vp_fx d2a, d2b, d2c, minD2;
            if (idx == outSel[0] || idx == outSel[1] || idx == outSel[2]) continue;
            d2a = vp_dist2_tangent(p, pA, t1, t2);
            d2b = vp_dist2_tangent(p, pB, t1, t2);
            d2c = vp_dist2_tangent(p, pC, t1, t2);
            minD2 = d2a;
            if (d2b < minD2) minD2 = d2b;
            if (d2c < minD2) minD2 = d2c;
            if (minD2 > bestMinD2) { bestMinD2 = minD2; best4 = idx; }
        }
        outSel[selCount++] = best4;
    }

    if (selCount > (vp_u16)VP_MANIFOLD_MAX_POINTS) selCount = (vp_u16)VP_MANIFOLD_MAX_POINTS;
    return selCount;
}

/* stable key assignment from pair cache */
static vp_u32 vp_assign_point_key(vpWorld* w, vpPairCacheEntry* pe, vpVec3 localA, vpVec3 localB, vp_u8* usedPrev)
{
    vp_u16 i;
    vp_u32 bestKey = 0;
    vp_fx bestDist2 = 0;
    vp_fx matchDist = (vp_fx)VP_MANIFOLD_MATCH_DIST;
    vp_fx matchDist2 = vp_fx_mul(matchDist, matchDist);

    for (i = 0; i < (vp_u16)VP_MANIFOLD_MAX_POINTS; ++i) {
        if (!pe->p[i].key) continue;
        if (usedPrev[i]) continue;

        /* require both localA and localB close (cheap) */
        {
            vp_fx d2a = vp_local_match_dist2(localA, pe->p[i].localA);
            if (d2a > matchDist2) continue;
            {
                vp_fx d2b = vp_local_match_dist2(localB, pe->p[i].localB);
                if (d2b > matchDist2) continue;
                {
                    vp_fx d2 = d2a + d2b;
                    if (bestKey == 0 || d2 < bestDist2) {
                        bestKey = pe->p[i].key;
                        bestDist2 = d2;
                        usedPrev[i] = 1; /* reserve */
                    }
                }
            }
        }
    }

    if (bestKey == 0) {
        bestKey = w->nextContactKey++;
    }
    return bestKey;
}

void vpContactsFinalize(vpWorld* w, vp_fx dt)
{
    vp_u16 i, j;
    vp_u16 outCount = 0;

    VP_UNUSED(dt);

    /* sort raw indices by pairKey */
    for (i = 0; i < w->contactRawCount; ++i) w->contactSort[i] = i;

    /* insertion sort (indices only) */
    for (i = 1; i < w->contactRawCount; ++i) {
        vp_u16 keyIdx = w->contactSort[i];
        vp_u32 keyPair = vp_pair_key(w->contactsRaw[keyIdx].bodyA, w->contactsRaw[keyIdx].bodyB);
        vp_u16 k = i;
        while (k > 0) {
            vp_u16 prevIdx = w->contactSort[k - 1];
            vp_u32 prevPair = vp_pair_key(w->contactsRaw[prevIdx].bodyA, w->contactsRaw[prevIdx].bodyB);
            if (prevPair <= keyPair) break;
            w->contactSort[k] = prevIdx;
            k--;
        }
        w->contactSort[k] = keyIdx;
    }

    /* walk groups */
    i = 0;
    while (i < w->contactRawCount) {
        vp_u16 first = w->contactSort[i];
        vp_u16 A = w->contactsRaw[first].bodyA;
        vp_u16 B = w->contactsRaw[first].bodyB;
        vp_u32 pairKey = vp_pair_key(A, B);

        /* gather candidates (cap) */
        vp_u16 candIdx[VP_MANIFOLD_CANDIDATES_MAX];
        vp_u16 candCount = 0;
        vpVec3 nSum = vpVec3_make(0,0,0);
        vp_fx fric = 0;
        vp_fx rest = 0;
        vp_u8 flagsOr = 0;

        while (i < w->contactRawCount) {
            vp_u16 idx = w->contactSort[i];
            vpContact* rc = &w->contactsRaw[idx];
            vp_u32 pk = vp_pair_key(rc->bodyA, rc->bodyB);
            if (pk != pairKey) break;

            /* clamp candidates per pair: keep the deepest ones */
            if (candCount < (vp_u16)VP_MANIFOLD_CANDIDATES_MAX) {
                candIdx[candCount++] = idx;
            } else {
                /* replace the shallowest penetration if this one is deeper */
                vp_u16 worst = 0;
                vp_fx worstPen = w->contactsRaw[candIdx[0]].penetration;
                for (j = 1; j < candCount; ++j) {
                    vp_fx pen = w->contactsRaw[candIdx[j]].penetration;
                    if (pen < worstPen) { worstPen = pen; worst = j; }
                }
                if (rc->penetration > worstPen) candIdx[worst] = idx;
            }

            nSum = vpVec3_add(nSum, rc->n);
            if (rc->friction > fric) fric = rc->friction;
            if (rc->restitution > rest) rest = rc->restitution;
            flagsOr = (vp_u8)(flagsOr | rc->flags);

            i++;
        }

        /* average normal */
        {
            vpVec3 normal = vpVec3_normalize(nSum);
            vp_u16 sel[VP_MANIFOLD_MAX_POINTS];
            vp_u16 selCount;
            vpPairCacheEntry* pe;

            if (normal.x == 0 && normal.y == 0 && normal.z == 0) {
                normal = w->contactsRaw[first].n;
            }

            /* ensure pair cache entry exists */
            pe = vp_find_pair_cache(w, pairKey);
            if (!pe->used || pe->pairKey != pairKey) {
                vp_u16 pi;
                if (pe->used && pe->active && pe->pairKey != pairKey)
                    vp_push_contact_event(w, VP_EVENT_CONTACT_END,
                        pe->bodyA, pe->bodyB, pe->pairKey);
                pe->used = 1;
                pe->pairKey = pairKey;
                pe->active = 0;
                pe->bodyA = A;
                pe->bodyB = B;
                pe->lastSeenStep = 0;
                for (pi = 0; pi < (vp_u16)VP_MANIFOLD_MAX_POINTS; ++pi) {
                    pe->p[pi].localA = vpVec3_make(0,0,0);
                    pe->p[pi].localB = vpVec3_make(0,0,0);
                    pe->p[pi].key = 0;
                }
            }

            /* contact begin/persist events per pair */
            if (!pe->active) vp_push_contact_event(w, VP_EVENT_CONTACT_BEGIN, A, B, pairKey);
            else vp_push_contact_event(w, VP_EVENT_CONTACT_PERSIST, A, B, pairKey);

            pe->active = 1;
            pe->lastSeenStep = w->stepId;

            /* sensor pairs: no solver contacts, just events */
            if (flagsOr & VP_CONTACT_FLAG_SENSOR) {
                vp_u16 pi;
                for (pi = 0; pi < (vp_u16)VP_MANIFOLD_MAX_POINTS; ++pi) {
                    pe->p[pi].key = 0;
                    pe->p[pi].localA = vpVec3_make(0,0,0);
                    pe->p[pi].localB = vpVec3_make(0,0,0);
                }
                continue;
            }

            /* reduce points */
            selCount = vp_select_manifold_points(w, candIdx, candCount, normal, sel);

            /* build solver contacts */
            {
                vp_u8 usedPrev[VP_MANIFOLD_MAX_POINTS];
                vp_u16 pi;
                for (pi = 0; pi < (vp_u16)VP_MANIFOLD_MAX_POINTS; ++pi) usedPrev[pi] = 0;

                for (pi = 0; pi < selCount; ++pi) {
                    vpContact* src = &w->contactsRaw[sel[pi]];
                    vpContact* dst;
                    vpBody* bA = vpWorldGetBody(w, (vpBodyId)A);
                    vpBody* bB = vpWorldGetBody(w, (vpBodyId)B);
                    vpVec3 localA, localB;
                    vp_u32 kAuto = 0;

                    if (outCount >= w->maxContacts) break;

                    /* sensor pairs produce no solver constraints */
                    if (flagsOr & VP_CONTACT_FLAG_SENSOR) continue;

                    dst = &w->contacts[outCount++];

                    dst->bodyA = A;
                    dst->bodyB = B;
                    dst->flags = flagsOr;
                    dst->_pad = 0;
                    dst->p = src->p;
                    dst->n = normal; /* stable manifold normal */
                    dst->penetration = src->penetration;
                    dst->friction = fric;
                    dst->restitution = rest;
                    dst->restitutionTarget = 0;

#if VP_ENABLE_SPLIT_IMPULSE
                    dst->lambdaBias = 0;
#endif

                    /* stable key: user key wins, else pair-cache based */
                    if (src->key) {
                        dst->key = src->key;
                    } else {
                        if (!bA) bA = &w->staticBody;
                        if (!bB) bB = &w->staticBody;

                        localA = vpQuat_rotate_inv_vec3(bA->rot, vpVec3_sub(dst->p, bA->pos));
                        localB = vpQuat_rotate_inv_vec3(bB->rot, vpVec3_sub(dst->p, bB->pos));
                        kAuto = vp_assign_point_key(w, pe, localA, localB, usedPrev);
                        dst->key = kAuto;

                        /* update pair cache point (in same slot 'pi' for this step) */
                        pe->p[pi].localA = localA;
                        pe->p[pi].localB = localB;
                        pe->p[pi].key = kAuto;
                    }

                    /* warm-start lookup */
                    {
                        vpContactCacheEntry* ce = vp_find_contact_cache(w, dst->key);
                        if (ce->used && ce->key == dst->key) {
                            dst->lambdaN = ce->lambdaN;
                            dst->lambdaT1 = ce->lambdaT1;
                            dst->lambdaT2 = ce->lambdaT2;
                        } else {
                            ce->used = 1;
                            ce->key = dst->key;
                            ce->lambdaN = 0;
                            ce->lambdaT1 = 0;
                            ce->lambdaT2 = 0;
                            dst->lambdaN = 0;
                            dst->lambdaT1 = 0;
                            dst->lambdaT2 = 0;
                        }
                    }
                }

                /* clear any remaining cached points not rewritten this step */
                for (pi = selCount; pi < (vp_u16)VP_MANIFOLD_MAX_POINTS; ++pi) {
                    pe->p[pi].key = 0;
                    pe->p[pi].localA = vpVec3_make(0,0,0);
                    pe->p[pi].localB = vpVec3_make(0,0,0);
                }
            }
        }
    }

    w->contactCount = outCount;

    /* generate contact END events for pairs not seen this step */
    for (i = 0; i < (vp_u16)VP_PAIR_CACHE_SIZE; ++i) {
        vpPairCacheEntry* pe = &w->pairCache[i];
        if (!pe->used) continue;
        if (!pe->active) continue;
        if (pe->lastSeenStep != w->stepId) {
            {
                vp_u16 pi;
                pe->active = 0;
                for (pi = 0; pi < (vp_u16)VP_MANIFOLD_MAX_POINTS; ++pi) {
                    pe->p[pi].key = 0;
                    pe->p[pi].localA = vpVec3_make(0,0,0);
                    pe->p[pi].localB = vpVec3_make(0,0,0);
                }
                vp_push_contact_event(w, VP_EVENT_CONTACT_END, pe->bodyA, pe->bodyB, pe->pairKey);
            }
        }
    }
}

/* commit solver lambdas to cache */
void vpContactsEnd(vpWorld* w)
{
    vp_u16 i;
    for (i = 0; i < w->contactCount; ++i) {
        vpContact* c = &w->contacts[i];
        vpContactCacheEntry* e;

        if (c->flags & VP_CONTACT_FLAG_SENSOR) continue;

        e = vp_find_contact_cache(w, c->key);
        e->used = 1;
        e->key = c->key;
        e->lambdaN = c->lambdaN;
        e->lambdaT1 = c->lambdaT1;
        e->lambdaT2 = c->lambdaT2;
    }
}
