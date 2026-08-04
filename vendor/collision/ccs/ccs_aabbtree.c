#include "ccs_aabbtree.h"

/* ============================================================
   Internal helpers
   ============================================================ */

static ccs_aabb aabb_merge(ccs_aabb a, ccs_aabb b)
{
    ccs_aabb out;

    out.min.x = (a.min.x < b.min.x) ? a.min.x : b.min.x;
    out.min.y = (a.min.y < b.min.y) ? a.min.y : b.min.y;
    out.min.z = (a.min.z < b.min.z) ? a.min.z : b.min.z;

    out.max.x = (a.max.x > b.max.x) ? a.max.x : b.max.x;
    out.max.y = (a.max.y > b.max.y) ? a.max.y : b.max.y;
    out.max.z = (a.max.z > b.max.z) ? a.max.z : b.max.z;

    return out;
}

static int aabb_overlap(const ccs_aabb* a, const ccs_aabb* b)
{
    if (a->max.x < b->min.x || a->min.x > b->max.x) return 0;
    if (a->max.y < b->min.y || a->min.y > b->max.y) return 0;
    if (a->max.z < b->min.z || a->min.z > b->max.z) return 0;
    return 1;
}

/* Build a very simple binary tree by splitting the array in half.
   (No SAH, no sorting.) */
static int build_node(
    CCS_AABBTree* tree,
    const ccs_aabb* boxes,
    const int* ids,
    int count
) {
    int node_idx;

    if (!tree || !boxes || !ids || count <= 0)
        return -1;

    if (tree->node_count >= CCS_AABBTREE_MAX_NODES)
        return -1;

    node_idx = tree->node_count;
    tree->node_count++;

    tree->nodes[node_idx].left = -1;
    tree->nodes[node_idx].right = -1;
    tree->nodes[node_idx].primitive_id = -1;
    tree->nodes[node_idx].box = boxes[0];

    if (count == 1) {
        tree->nodes[node_idx].primitive_id = ids[0];
        return node_idx;
    }

    {
        int mid;
        int left_idx;
        int right_idx;
        ccs_aabb left_box;
        ccs_aabb right_box;

        mid = count / 2;
        if (mid < 1)
            mid = 1;

        left_idx = build_node(tree, boxes, ids, mid);
        right_idx = build_node(tree, boxes + mid, ids + mid, count - mid);

        tree->nodes[node_idx].left = left_idx;
        tree->nodes[node_idx].right = right_idx;

        /* compute bounds */
        left_box = (left_idx >= 0) ? tree->nodes[left_idx].box : boxes[0];
        right_box = (right_idx >= 0) ? tree->nodes[right_idx].box : boxes[count - 1];

        tree->nodes[node_idx].box = aabb_merge(left_box, right_box);

        /* leaf safety if recursion failed */
        if (left_idx < 0 && right_idx < 0) {
            tree->nodes[node_idx].primitive_id = ids[0];
        }

    }

    return node_idx;
}

/* ============================================================
   API
   ============================================================ */

void ccs_aabbtree_init(CCS_AABBTree* tree)
{
    if (!tree)
        return;

    tree->node_count = 0;
    tree->root = -1;
}

int ccs_aabbtree_build(
    CCS_AABBTree* tree,
    const ccs_aabb* boxes,
    const int* ids,
    int count
) {
    if (!tree || !boxes || !ids || count <= 0)
        return 0;

    ccs_aabbtree_init(tree);

    tree->root = build_node(tree, boxes, ids, count);
    return (tree->root >= 0);
}

int ccs_aabbtree_query_aabb(
    const CCS_AABBTree* tree,
    const ccs_aabb* query,
    int* out_ids,
    int max_ids
) {
    int stack[CCS_AABBTREE_MAX_NODES];
    int sp;
    int out_count;

    if (!tree || !query || !out_ids || max_ids <= 0)
        return 0;

    if (tree->root < 0)
        return 0;

    sp = 0;
    out_count = 0;

    stack[sp++] = tree->root;

    while (sp > 0) {
        int idx;
        const CCS_AABBNode* node;

        idx = stack[--sp];
        if (idx < 0 || idx >= tree->node_count)
            continue;

        node = &tree->nodes[idx];

        if (!aabb_overlap(&node->box, query))
            continue;

        if (node->primitive_id >= 0) {
            if (out_count < max_ids) {
                out_ids[out_count++] = node->primitive_id;
            }
        } else {
            if (node->left >= 0 && sp < CCS_AABBTREE_MAX_NODES)
                stack[sp++] = node->left;
            if (node->right >= 0 && sp < CCS_AABBTREE_MAX_NODES)
                stack[sp++] = node->right;
        }
    }

    return out_count;
}
