#ifndef CCS_AABBTREE_H
#define CCS_AABBTREE_H

/*
    AABB Tree (muy simple)
    ----------------------
    Drop-in para que compile con los tipos reales del repo.

    Nota: este árbol es un builder/query muy sencillo; NO hace rotaciones
    ni incremental updates.

    Importante (C89):
    - Los structs tienen TAG para permitir forward declarations.
*/

#include "ccs_shapes.h" /* ccs_aabb */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCS_AABBTREE_MAX_NODES
#define CCS_AABBTREE_MAX_NODES 512
#endif

typedef struct CCS_AABBNode {
    ccs_aabb box;
    int left;
    int right;
    int primitive_id; /* -1 si no es hoja */
} CCS_AABBNode;

struct CCS_AABBTree {
    CCS_AABBNode nodes[CCS_AABBTREE_MAX_NODES];
    int node_count;
    int root;
};

/* API */
void ccs_aabbtree_init(CCS_AABBTree* tree);
int  ccs_aabbtree_build(
    CCS_AABBTree* tree,
    const ccs_aabb* boxes,
    const int* ids,
    int count
);

/* Queries */
int ccs_aabbtree_query_aabb(
    const CCS_AABBTree* tree,
    const ccs_aabb* query,
    int* out_ids,
    int max_ids
);

#ifdef __cplusplus
}
#endif

#endif /* CCS_AABBTREE_H */
