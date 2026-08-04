#ifndef VP_TOTAL_SOLVER_H
#define VP_TOTAL_SOLVER_H

#include "vp_world.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef vp_u16 vpTransformNodeId;
typedef vp_u16 vpTransformConstraintId;

typedef struct vpTransform {
    vpVec3 position;
    vpQuat rotation;
    vpVec3 scale;
} vpTransform;

typedef enum vpTransformAuthority {
    VP_TRANSFORM_AUTH_MANUAL = 0,
    VP_TRANSFORM_AUTH_EXTERNAL = 1,
    VP_TRANSFORM_AUTH_PHYSICS = 2,
    VP_TRANSFORM_AUTH_BLEND = 3
} vpTransformAuthority;

typedef enum vpTransformConstraintType {
    VP_TRANSFORM_CONSTRAINT_PARENT = 1,
    VP_TRANSFORM_CONSTRAINT_COPY_POSITION = 2,
    VP_TRANSFORM_CONSTRAINT_COPY_ROTATION = 3,
    VP_TRANSFORM_CONSTRAINT_COPY_SCALE = 4,
    VP_TRANSFORM_CONSTRAINT_LOOK_AT = 5,
    VP_TRANSFORM_CONSTRAINT_DISTANCE = 6,
    VP_TRANSFORM_CONSTRAINT_POSITION_LIMIT = 7,
    VP_TRANSFORM_CONSTRAINT_FOLLOW = 8
} vpTransformConstraintType;

typedef enum vpTransformPhase {
    VP_TRANSFORM_PHASE_PRE_PHYSICS = 1,
    VP_TRANSFORM_PHASE_POST_PHYSICS = 2,
    VP_TRANSFORM_PHASE_BOTH = 3
} vpTransformPhase;

#define VP_TRANSFORM_NODE_READ_EXTERNAL   0x01U
#define VP_TRANSFORM_NODE_WRITE_EXTERNAL  0x02U
#define VP_TRANSFORM_NODE_DRIVE_DYNAMIC   0x04U
#define VP_TRANSFORM_NODE_ENABLED         0x80U

#define VP_TRANSFORM_MATH_COMPOSE         0x00000001UL
#define VP_TRANSFORM_MATH_INVERSE         0x00000002UL
#define VP_TRANSFORM_MATH_POINT           0x00000004UL
#define VP_TRANSFORM_MATH_ROTATE          0x00000008UL
#define VP_TRANSFORM_MATH_QUAT_MUL        0x00000010UL
#define VP_TRANSFORM_MATH_QUAT_NORMALIZE  0x00000020UL
#define VP_TRANSFORM_MATH_BLEND           0x00000040UL
#define VP_TRANSFORM_MATH_LOOK_AT         0x00000080UL

typedef struct vpTransformMathProvider {
    void* user;
    vp_u32 capabilities;
    void (*compose)(void* user, vpTransform* out, const vpTransform* parent, const vpTransform* local);
    void (*inverse)(void* user, vpTransform* out, const vpTransform* in);
    void (*transformPoint)(void* user, vpVec3* out, const vpTransform* transform, const vpVec3* point);
    void (*rotateVector)(void* user, vpVec3* out, const vpQuat* rotation, const vpVec3* vector);
    void (*quatMul)(void* user, vpQuat* out, const vpQuat* a, const vpQuat* b);
    void (*quatNormalize)(void* user, vpQuat* out, const vpQuat* q);
    void (*blend)(void* user, vpTransform* out, const vpTransform* a, const vpTransform* b, vp_fx weight);
    void (*lookAt)(void* user, vpQuat* out, const vpVec3* origin, const vpVec3* target, const vpVec3* up);
} vpTransformMathProvider;

typedef struct vpTransformProvider {
    void* user;
    int (*readLocal)(void* user, vp_u32 externalId, vpTransform* out);
    int (*readWorld)(void* user, vp_u32 externalId, vpTransform* out);
    int (*writeLocal)(void* user, vp_u32 externalId, const vpTransform* value);
    int (*writeWorld)(void* user, vp_u32 externalId, const vpTransform* value);
    int (*getParent)(void* user, vp_u32 externalId, vp_u32* outParentExternalId);
} vpTransformProvider;

struct vpTotalSolver;
typedef struct vpTotalSolver vpTotalSolver;

typedef struct vpCollisionProvider {
    void* user;
    void (*beginStep)(void* user, vpTotalSolver* solver, vpWorld* world, vp_fx dt);
    void (*generateContacts)(void* user, vpTotalSolver* solver, vpWorld* world, vp_fx dt);
    void (*endStep)(void* user, vpTotalSolver* solver, vpWorld* world, vp_fx dt);
    vp_fx (*bodySweepTOI)(void* user, vp_u16 bodyId,
        vpVec3 fromPos, vpQuat fromRot,
        vpVec3 toPos, vpQuat toRot,
        vp_u16* hitBody, vpVec3* hitPoint, vpVec3* hitNormal);
} vpCollisionProvider;

typedef struct vpTotalSolverDesc {
    vp_u16 maxTransformNodes;
    vp_u16 maxTransformConstraints;
    vp_u16 transformIterations;
} vpTotalSolverDesc;

typedef struct vpTransformNode {
    vp_u8 used;
    vp_u8 flags;
    vp_u8 authority;
    vp_u8 dirty;
    vp_u16 parent;
    vp_u16 body;
    vp_u32 externalId;
    vp_fx blendWeight;
    vpTransform local;
    vpTransform world;
    vpTransform externalWorld;
} vpTransformNode;

typedef struct vpTransformConstraint {
    vp_u8 used;
    vp_u8 enabled;
    vp_u8 type;
    vp_u8 phase;
    vp_u8 priority;
    vp_u8 _pad0;
    vp_u16 target;
    vp_u16 source;
    vp_fx weight;
    vpTransform offset;
    vpVec3 minValue;
    vpVec3 maxValue;
    vp_fx distance;
} vpTransformConstraint;

struct vpTotalSolver {
    vpWorld* world;
    vpTransformNode* nodes;
    vpTransformConstraint* constraints;
    vp_u16* constraintOrder;
    vp_u8* visitState;
    vp_u16 maxNodes;
    vp_u16 maxConstraints;
    vp_u16 iterations;
    vp_u16 nodeCount;
    vp_u16 constraintCount;
    vp_u16 cycleCount;
    vp_u16 providerErrorCount;
    vpTransformMathProvider math;
    vpTransformProvider transforms;
    vpCollisionProvider collisions;
    vpCallbacks legacyCallbacks;
    vp_u8 legacyCallbacksEnabled;
    vp_u8 attached;
    vp_u8 orderDirty;
    vp_u8 inCallback;
};

vpTransform vpTransformIdentity(void);

vp_u32 vpTotalSolverMemSize(const vpTotalSolverDesc* desc);
vpTotalSolver* vpTotalSolverInit(void* mem, vp_u32 memBytes, vpWorld* world, const vpTotalSolverDesc* desc);

void vpTotalSolverSetMathProvider(vpTotalSolver* solver, const vpTransformMathProvider* provider);
void vpTotalSolverSetTransformProvider(vpTotalSolver* solver, const vpTransformProvider* provider);
void vpTotalSolverSetCollisionProvider(vpTotalSolver* solver, const vpCollisionProvider* provider);
void vpTotalSolverSetLegacyCallbacks(vpTotalSolver* solver, const vpCallbacks* callbacks);

/* Provider-backed math service. Every operation has an internal fixed-point fallback. */
void vpTotalSolverMathCompose(vpTotalSolver* solver, vpTransform* out, const vpTransform* parent, const vpTransform* local);
void vpTotalSolverMathInverse(vpTotalSolver* solver, vpTransform* out, const vpTransform* value);
void vpTotalSolverMathTransformPoint(vpTotalSolver* solver, vpVec3* out, const vpTransform* transform, const vpVec3* point);
void vpTotalSolverMathRotateVector(vpTotalSolver* solver, vpVec3* out, const vpQuat* rotation, const vpVec3* vector);
void vpTotalSolverMathQuatMul(vpTotalSolver* solver, vpQuat* out, const vpQuat* a, const vpQuat* b);
void vpTotalSolverMathQuatNormalize(vpTotalSolver* solver, vpQuat* out, const vpQuat* value);
void vpTotalSolverMathBlend(vpTotalSolver* solver, vpTransform* out, const vpTransform* a, const vpTransform* b, vp_fx weight);
void vpTotalSolverMathLookAt(vpTotalSolver* solver, vpQuat* out, const vpVec3* origin, const vpVec3* target, const vpVec3* up);

int  vpTotalSolverAttach(vpTotalSolver* solver);
void vpTotalSolverDetach(vpTotalSolver* solver);
void vpTotalSolverStep(vpTotalSolver* solver, vp_fx dt);
void vpTotalSolverStepCCD(vpTotalSolver* solver, vp_fx dt, vp_u16 maxSubSteps);

vpTransformNodeId vpTransformNodeCreate(vpTotalSolver* solver, vp_u32 externalId);
void vpTransformNodeDestroy(vpTotalSolver* solver, vpTransformNodeId node);
vpTransformNode* vpTransformNodeGet(vpTotalSolver* solver, vpTransformNodeId node);
void vpTransformNodeSetParent(vpTotalSolver* solver, vpTransformNodeId node, vpTransformNodeId parent);
void vpTransformNodeBindBody(vpTotalSolver* solver, vpTransformNodeId node, vpBodyId body, vpTransformAuthority authority);
void vpTransformNodeSetAuthority(vpTotalSolver* solver, vpTransformNodeId node, vpTransformAuthority authority, vp_fx blendWeight);
void vpTransformNodeSetLocal(vpTotalSolver* solver, vpTransformNodeId node, const vpTransform* value);
void vpTransformNodeSetWorld(vpTotalSolver* solver, vpTransformNodeId node, const vpTransform* value);

vpTransformConstraintId vpTransformConstraintCreate(vpTotalSolver* solver,
    vpTransformConstraintType type, vpTransformPhase phase,
    vpTransformNodeId target, vpTransformNodeId source, vp_u8 priority);
vpTransformConstraint* vpTransformConstraintGet(vpTotalSolver* solver, vpTransformConstraintId id);
void vpTransformConstraintDestroy(vpTotalSolver* solver, vpTransformConstraintId id);
void vpTransformConstraintSetWeight(vpTotalSolver* solver, vpTransformConstraintId id, vp_fx weight);
void vpTransformConstraintSetOffset(vpTotalSolver* solver, vpTransformConstraintId id, const vpTransform* offset);
void vpTransformConstraintSetLimits(vpTotalSolver* solver, vpTransformConstraintId id, vpVec3 minValue, vpVec3 maxValue);
void vpTransformConstraintSetDistance(vpTotalSolver* solver, vpTransformConstraintId id, vp_fx distance);

/* Manual stages are exposed for hosts that need a custom frame pipeline. */
void vpTotalSolverPullTransforms(vpTotalSolver* solver);
void vpTotalSolverSolvePrePhysics(vpTotalSolver* solver, vp_fx dt);
void vpTotalSolverSolvePostPhysics(vpTotalSolver* solver, vp_fx dt);
void vpTotalSolverPushTransforms(vpTotalSolver* solver);

#ifdef __cplusplus
}
#endif
#endif
