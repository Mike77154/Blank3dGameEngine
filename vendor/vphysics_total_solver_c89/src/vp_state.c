#include "vp_state.h"
#include "vp_world.h"

/* no libc dependency */
static void vp_memcpy(void* dst, const void* src, vp_u32 bytes)
{
    vp_u32 i;
    vp_u8* d = (vp_u8*)dst;
    const vp_u8* s = (const vp_u8*)src;
    for (i = 0; i < bytes; ++i) d[i] = s[i];
}

#define VP_STATE_MAGIC 0x54535056u /* 'VPST' little-endian */

typedef struct vpStateHeader {
    vp_u32 magic;
    vp_u32 version;

    vp_u16 maxBodies;
    vp_u16 maxContacts;
    vp_u16 maxJoints;
    vp_u16 solverIters;

    vp_u32 stepId;
    vp_u32 nextContactKey;

    vpVec3 gravity;
    vp_fx defaultFriction;
    vp_fx defaultRestitution;

    vp_u8 waterEnabled;
    vp_u8 _pad0;
    vp_u16 _pad1;
    vp_fx waterY;
    vp_fx waterDensity;

    vp_u16 contactCount;
    vp_u16 contactRawCount;
    vp_u16 eventCount;
    vp_u16 _pad2;
} vpStateHeader;

vp_u32 vpWorldStateSize(const vpWorld* world)
{
    const vpWorld* w = world;
    vp_u32 bytes = 0;
    if (!w) return 0;

    bytes += (vp_u32)sizeof(vpStateHeader);
    bytes += (vp_u32)sizeof(vpBody) * (vp_u32)w->maxBodies;
    bytes += (vp_u32)sizeof(vpContact) * (vp_u32)w->maxContacts;
    bytes += (vp_u32)sizeof(vpJoint) * (vp_u32)w->maxJoints;
    bytes += (vp_u32)sizeof(vpContactCacheEntry) * (vp_u32)VP_CONTACT_CACHE_SIZE;
    bytes += (vp_u32)sizeof(vpPairCacheEntry)   * (vp_u32)VP_PAIR_CACHE_SIZE;
    return bytes;
}

vp_u8 vpWorldSaveState(const vpWorld* world, void* outBytes, vp_u32 outCap)
{
    const vpWorld* w = world;
    vpStateHeader h;
    vp_u8* p;

    if (!w || !outBytes) return 0;
    if (outCap < vpWorldStateSize(w)) return 0;

    h.magic = VP_STATE_MAGIC;
    h.version = 1;
    h.maxBodies = w->maxBodies;
    h.maxContacts = w->maxContacts;
    h.maxJoints = w->maxJoints;
    h.solverIters = w->solverIters;
    h.stepId = w->stepId;
    h.nextContactKey = w->nextContactKey;
    h.gravity = w->gravity;
    h.defaultFriction = w->defaultFriction;
    h.defaultRestitution = w->defaultRestitution;
    h.waterEnabled = w->waterEnabled;
    h._pad0 = 0;
    h._pad1 = 0;
    h.waterY = w->waterY;
    h.waterDensity = w->waterDensity;
    h.contactCount = w->contactCount;
    h.contactRawCount = w->contactRawCount;
    h.eventCount = w->eventCount;
    h._pad2 = 0;

    p = (vp_u8*)outBytes;
    vp_memcpy(p, &h, (vp_u32)sizeof(h));
    p += sizeof(h);

    vp_memcpy(p, w->bodies, (vp_u32)sizeof(vpBody) * (vp_u32)w->maxBodies);
    p += sizeof(vpBody) * (vp_u32)w->maxBodies;

    vp_memcpy(p, w->contacts, (vp_u32)sizeof(vpContact) * (vp_u32)w->maxContacts);
    p += sizeof(vpContact) * (vp_u32)w->maxContacts;

    vp_memcpy(p, w->joints, (vp_u32)sizeof(vpJoint) * (vp_u32)w->maxJoints);
    p += sizeof(vpJoint) * (vp_u32)w->maxJoints;

    vp_memcpy(p, w->contactCache, (vp_u32)sizeof(vpContactCacheEntry) * (vp_u32)VP_CONTACT_CACHE_SIZE);
    p += sizeof(vpContactCacheEntry) * (vp_u32)VP_CONTACT_CACHE_SIZE;

    vp_memcpy(p, w->pairCache, (vp_u32)sizeof(vpPairCacheEntry) * (vp_u32)VP_PAIR_CACHE_SIZE);
    p += sizeof(vpPairCacheEntry) * (vp_u32)VP_PAIR_CACHE_SIZE;

    return 1;
}

vp_u8 vpWorldLoadState(vpWorld* w, const void* inBytes, vp_u32 inCap)
{
    vpStateHeader h;
    const vp_u8* p;

    if (!w || !inBytes) return 0;
    if (inCap < (vp_u32)sizeof(vpStateHeader)) return 0;

    p = (const vp_u8*)inBytes;
    vp_memcpy(&h, p, (vp_u32)sizeof(h));
    p += sizeof(h);

    if (h.magic != VP_STATE_MAGIC) return 0;
    if (h.version != 1) return 0;
    if (h.maxBodies != w->maxBodies) return 0;
    if (h.maxContacts != w->maxContacts) return 0;
    if (h.maxJoints != w->maxJoints) return 0;

    if (inCap < vpWorldStateSize(w)) return 0;

    w->solverIters = h.solverIters;
    w->stepId = h.stepId;
    w->nextContactKey = h.nextContactKey;
    w->gravity = h.gravity;
    w->defaultFriction = h.defaultFriction;
    w->defaultRestitution = h.defaultRestitution;
    w->waterEnabled = h.waterEnabled;
    w->waterY = h.waterY;
    w->waterDensity = h.waterDensity;
    w->contactCount = h.contactCount;
    w->contactRawCount = h.contactRawCount;
    w->eventCount = 0; /* events are step-local; drop */

    vp_memcpy(w->bodies, p, (vp_u32)sizeof(vpBody) * (vp_u32)w->maxBodies);
    p += sizeof(vpBody) * (vp_u32)w->maxBodies;

    vp_memcpy(w->contacts, p, (vp_u32)sizeof(vpContact) * (vp_u32)w->maxContacts);
    p += sizeof(vpContact) * (vp_u32)w->maxContacts;

    vp_memcpy(w->joints, p, (vp_u32)sizeof(vpJoint) * (vp_u32)w->maxJoints);
    p += sizeof(vpJoint) * (vp_u32)w->maxJoints;

    vp_memcpy(w->contactCache, p, (vp_u32)sizeof(vpContactCacheEntry) * (vp_u32)VP_CONTACT_CACHE_SIZE);
    p += sizeof(vpContactCacheEntry) * (vp_u32)VP_CONTACT_CACHE_SIZE;

    vp_memcpy(w->pairCache, p, (vp_u32)sizeof(vpPairCacheEntry) * (vp_u32)VP_PAIR_CACHE_SIZE);
    p += sizeof(vpPairCacheEntry) * (vp_u32)VP_PAIR_CACHE_SIZE;

    /* reset transients */
    w->contactRawCount = 0;
    w->eventCount = 0;

#if VP_ENABLE_ISLANDS
    w->linkCount = 0;
#endif

    return 1;
}
