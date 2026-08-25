#include "giff_internal.h"

typedef struct giff_quant_box {
    giff_u16 start;
    giff_u16 count;
    giff_u8 rmin;
    giff_u8 rmax;
    giff_u8 gmin;
    giff_u8 gmax;
    giff_u8 bmin;
    giff_u8 bmax;
    giff_u32 weight;
} giff_quant_box;

static giff_u8 giff_clamp_u8(giff_s32 value)
{
    if (value < 0) {
        return 0u;
    }
    if (value > 255) {
        return 255u;
    }
    return (giff_u8)value;
}

static giff_u8 giff_bin_r5(giff_u16 bin)
{
    return (giff_u8)((bin >> 10u) & 31u);
}

static giff_u8 giff_bin_g5(giff_u16 bin)
{
    return (giff_u8)((bin >> 5u) & 31u);
}

static giff_u8 giff_bin_b5(giff_u16 bin)
{
    return (giff_u8)(bin & 31u);
}

static giff_u8 giff_5_to_8(giff_u8 value)
{
    return (giff_u8)((((giff_u16)value) * 255u + 15u) / 31u);
}

static giff_u32 giff_color_distance(
    const giff_rgb8* c,
    giff_s32 r,
    giff_s32 g,
    giff_s32 b
)
{
    giff_s32 dr;
    giff_s32 dg;
    giff_s32 db;
    giff_u32 wr;
    giff_u32 wg;
    giff_u32 wb;

    dr = r - (giff_s32)c->r;
    dg = g - (giff_s32)c->g;
    db = b - (giff_s32)c->b;

    wr = (giff_u32)299u;
    wg = (giff_u32)587u;
    wb = (giff_u32)114u;

    return (giff_u32)(wr * (giff_u32)(dr * dr) +
                      wg * (giff_u32)(dg * dg) +
                      wb * (giff_u32)(db * db));
}

static giff_s32 giff_ordered_delta(giff_u16 x, giff_u16 y)
{
    static const giff_u8 k_bayer4x4[16] = {
        0u, 8u, 2u, 10u,
        12u, 4u, 14u, 6u,
        3u, 11u, 1u, 9u,
        15u, 7u, 13u, 5u
    };
    giff_u8 t;
    giff_fx16_16 offset_fx;
    giff_fx16_16 scale_fx;
    giff_fx16_16 delta_fx;

    t = k_bayer4x4[((giff_u32)(y & 3u) << 2u) | (giff_u32)(x & 3u)];
    offset_fx = (giff_fx16_16)(((giff_s32)t * 2L - 15L) * GIFF_FX_ONE / 32L);
    scale_fx = GIFF_FX_FROM_INT(24);
    delta_fx = (giff_fx16_16)((offset_fx * scale_fx) / GIFF_FX_ONE);
    return (giff_s32)GIFF_FX_TO_INT(delta_fx);
}

static void giff_quant_box_reset(giff_quant_box* box)
{
    if (box == 0) {
        return;
    }
    box->rmin = 31u;
    box->gmin = 31u;
    box->bmin = 31u;
    box->rmax = 0u;
    box->gmax = 0u;
    box->bmax = 0u;
    box->weight = 0u;
}

static void giff_quant_box_recompute(
    giff_encoder* enc,
    const giff_u16* bins,
    giff_quant_box* box
)
{
    giff_u16 i;

    giff_quant_box_reset(box);
    if (enc == 0 || bins == 0 || box == 0 || box->count == 0u) {
        return;
    }

    for (i = 0u; i < box->count; ++i) {
        giff_u16 bin;
        giff_u32 count;
        giff_u8 r5;
        giff_u8 g5;
        giff_u8 b5;

        bin = bins[(giff_u16)(box->start + i)];
        count = enc->quant_hist[bin];
        if (count == 0u) {
            continue;
        }

        r5 = giff_bin_r5(bin);
        g5 = giff_bin_g5(bin);
        b5 = giff_bin_b5(bin);

        if (r5 < box->rmin) box->rmin = r5;
        if (r5 > box->rmax) box->rmax = r5;
        if (g5 < box->gmin) box->gmin = g5;
        if (g5 > box->gmax) box->gmax = g5;
        if (b5 < box->bmin) box->bmin = b5;
        if (b5 > box->bmax) box->bmax = b5;
        box->weight += count;
    }

    if (box->weight == 0u) {
        box->rmin = 0u;
        box->gmin = 0u;
        box->bmin = 0u;
        box->rmax = 0u;
        box->gmax = 0u;
        box->bmax = 0u;
    }
}

static giff_u8 giff_quant_box_axis(const giff_quant_box* box)
{
    giff_u8 rr;
    giff_u8 gr;
    giff_u8 br;

    if (box == 0) {
        return 0u;
    }

    rr = (giff_u8)(box->rmax - box->rmin);
    gr = (giff_u8)(box->gmax - box->gmin);
    br = (giff_u8)(box->bmax - box->bmin);

    if (gr >= rr && gr >= br) {
        return 1u;
    }
    if (br >= rr && br >= gr) {
        return 2u;
    }
    return 0u;
}

static giff_u8 giff_quant_box_range(const giff_quant_box* box)
{
    giff_u8 rr;
    giff_u8 gr;
    giff_u8 br;
    giff_u8 best;

    if (box == 0) {
        return 0u;
    }

    rr = (giff_u8)(box->rmax - box->rmin);
    gr = (giff_u8)(box->gmax - box->gmin);
    br = (giff_u8)(box->bmax - box->bmin);
    best = rr;
    if (gr > best) {
        best = gr;
    }
    if (br > best) {
        best = br;
    }
    return best;
}

static giff_u8 giff_quant_axis_value(giff_u16 bin, giff_u8 axis)
{
    if (axis == 1u) {
        return giff_bin_g5(bin);
    }
    if (axis == 2u) {
        return giff_bin_b5(bin);
    }
    return giff_bin_r5(bin);
}

static void giff_quant_sort_bins(
    giff_u16* bins,
    giff_u16* tmp,
    giff_u16 start,
    giff_u16 count,
    giff_u8 axis
)
{
    giff_u16 i;
    giff_u16 offsets[32];
    giff_u16 counts[32];
    giff_u16 total;

    for (i = 0u; i < 32u; ++i) {
        counts[i] = 0u;
        offsets[i] = 0u;
    }

    for (i = 0u; i < count; ++i) {
        giff_u8 v;
        v = giff_quant_axis_value(bins[(giff_u16)(start + i)], axis);
        counts[v] = (giff_u16)(counts[v] + 1u);
    }

    total = 0u;
    for (i = 0u; i < 32u; ++i) {
        offsets[i] = total;
        total = (giff_u16)(total + counts[i]);
    }

    for (i = 0u; i < count; ++i) {
        giff_u16 bin;
        giff_u8 v;
        giff_u16 pos;

        bin = bins[(giff_u16)(start + i)];
        v = giff_quant_axis_value(bin, axis);
        pos = offsets[v];
        tmp[pos] = bin;
        offsets[v] = (giff_u16)(offsets[v] + 1u);
    }

    for (i = 0u; i < count; ++i) {
        bins[(giff_u16)(start + i)] = tmp[i];
    }
}

static giff_result giff_quant_split_box(
    giff_encoder* enc,
    giff_u16* bins,
    giff_u16* tmp,
    giff_quant_box* boxes,
    giff_u16* io_used
)
{
    giff_u16 box_index;
    giff_u16 used;
    giff_u16 i;
    giff_u8 best_range;
    giff_u32 best_weight;
    giff_u8 axis;
    giff_u32 half_weight;
    giff_u32 acc;
    giff_u16 left_count;
    giff_quant_box left;
    giff_quant_box right;

    if (enc == 0 || bins == 0 || tmp == 0 || boxes == 0 || io_used == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    used = *io_used;
    box_index = 0xFFFFu;
    best_range = 0u;
    best_weight = 0u;

    for (i = 0u; i < used; ++i) {
        giff_u8 range;
        if (boxes[i].count < 2u || boxes[i].weight == 0u) {
            continue;
        }
        range = giff_quant_box_range(&boxes[i]);
        if (range == 0u) {
            continue;
        }
        if (box_index == 0xFFFFu || range > best_range ||
            (range == best_range && boxes[i].weight > best_weight)) {
            box_index = i;
            best_range = range;
            best_weight = boxes[i].weight;
        }
    }

    if (box_index == 0xFFFFu) {
        return GIFF_E_DONE;
    }

    axis = giff_quant_box_axis(&boxes[box_index]);
    giff_quant_sort_bins(bins, tmp, boxes[box_index].start, boxes[box_index].count, axis);

    half_weight = boxes[box_index].weight / 2u;
    acc = 0u;
    left_count = 0u;

    for (i = 0u; i < boxes[box_index].count; ++i) {
        giff_u16 bin;
        bin = bins[(giff_u16)(boxes[box_index].start + i)];
        acc += enc->quant_hist[bin];
        left_count = (giff_u16)(i + 1u);
        if (acc >= half_weight) {
            break;
        }
    }

    if (left_count == 0u) {
        left_count = 1u;
    }
    if (left_count >= boxes[box_index].count) {
        left_count = (giff_u16)(boxes[box_index].count / 2u);
        if (left_count == 0u) {
            left_count = 1u;
        }
    }

    left = boxes[box_index];
    left.count = left_count;
    right.start = (giff_u16)(boxes[box_index].start + left_count);
    right.count = (giff_u16)(boxes[box_index].count - left_count);
    right.rmin = right.rmax = right.gmin = right.gmax = right.bmin = right.bmax = 0u;
    right.weight = 0u;

    if (right.count == 0u) {
        return GIFF_E_DONE;
    }

    giff_quant_box_recompute(enc, bins, &left);
    giff_quant_box_recompute(enc, bins, &right);

    boxes[box_index] = left;
    boxes[used] = right;
    *io_used = (giff_u16)(used + 1u);
    return GIFF_OK;
}

static void giff_quant_box_to_palette(
    giff_encoder* enc,
    const giff_u16* bins,
    const giff_quant_box* box,
    giff_rgb8* out
)
{
    giff_u16 i;
    giff_u32 total;
    giff_u32 rsum;
    giff_u32 gsum;
    giff_u32 bsum;

    if (enc == 0 || bins == 0 || box == 0 || out == 0 || box->count == 0u) {
        if (out != 0) {
            out->r = 0u;
            out->g = 0u;
            out->b = 0u;
        }
        return;
    }

    total = 0u;
    rsum = 0u;
    gsum = 0u;
    bsum = 0u;

    for (i = 0u; i < box->count; ++i) {
        giff_u16 bin;
        giff_u32 count;
        giff_u8 r5;
        giff_u8 g5;
        giff_u8 b5;

        bin = bins[(giff_u16)(box->start + i)];
        count = enc->quant_hist[bin];
        if (count == 0u) {
            continue;
        }
        r5 = giff_bin_r5(bin);
        g5 = giff_bin_g5(bin);
        b5 = giff_bin_b5(bin);
        rsum += (giff_u32)r5 * count;
        gsum += (giff_u32)g5 * count;
        bsum += (giff_u32)b5 * count;
        total += count;
    }

    if (total == 0u) {
        out->r = 0u;
        out->g = 0u;
        out->b = 0u;
        return;
    }

    out->r = giff_5_to_8((giff_u8)((rsum + total / 2u) / total));
    out->g = giff_5_to_8((giff_u8)((gsum + total / 2u) / total));
    out->b = giff_5_to_8((giff_u8)((bsum + total / 2u) / total));
}

giff_result giff_palette_find_nearest(
    const giff_rgb8* palette,
    giff_u16 palette_entries,
    giff_u8 transparent_index,
    giff_u8 has_transparent,
    giff_s32 r,
    giff_s32 g,
    giff_s32 b,
    giff_u8* out_index,
    giff_rgb8* out_color
)
{
    giff_u16 i;
    giff_u16 best;
    giff_u32 best_dist;

    if (palette == 0 || out_index == 0 || palette_entries == 0u) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    r = (giff_s32)giff_clamp_u8(r);
    g = (giff_s32)giff_clamp_u8(g);
    b = (giff_s32)giff_clamp_u8(b);

    best = 0u;
    best_dist = 0xFFFFFFFFUL;

    for (i = 0u; i < palette_entries; ++i) {
        giff_u32 dist;
        if (has_transparent && i == (giff_u16)transparent_index) {
            continue;
        }
        dist = giff_color_distance(&palette[i], r, g, b);
        if (dist < best_dist) {
            best_dist = dist;
            best = i;
            if (dist == 0u) {
                break;
            }
        }
    }

    *out_index = (giff_u8)best;
    if (out_color != 0) {
        *out_color = palette[best];
    }
    return GIFF_OK;
}

static void giff_octree_init_node(giff_oct_node* node, giff_u8 level, giff_u8 is_leaf)
{
    giff_u16 i;

    if (node == 0) {
        return;
    }

    for (i = 0u; i < 8u; ++i) {
        node->child[i] = GIFF_OCTREE_INVALID;
    }
    node->next_reducible = GIFF_OCTREE_INVALID;
    node->palette_index = GIFF_OCTREE_INVALID;
    node->level = level;
    node->is_leaf = is_leaf;
    node->count = 0u;
    node->rsum = 0u;
    node->gsum = 0u;
    node->bsum = 0u;
}

static giff_u8 giff_octree_child_index_rgb(giff_u8 r, giff_u8 g, giff_u8 b, giff_u8 level)
{
    giff_u8 shift;
    giff_u8 index;

    shift = (giff_u8)(7u - level);
    index = 0u;
    index = (giff_u8)(index | (giff_u8)(((r >> shift) & 1u) << 2u));
    index = (giff_u8)(index | (giff_u8)(((g >> shift) & 1u) << 1u));
    index = (giff_u8)(index | (giff_u8)((b >> shift) & 1u));
    return index;
}

static giff_u8 giff_octree_child_index(const giff_rgba8* px, giff_u8 level)
{
    return giff_octree_child_index_rgb(px->r, px->g, px->b, level);
}

static giff_u16 giff_octree_new_node(
    giff_oct_node* nodes,
    giff_u16* io_used,
    giff_u16* reducible_heads,
    giff_u8 level,
    giff_u8 is_leaf
)
{
    giff_u16 index;

    if (nodes == 0 || io_used == 0) {
        return GIFF_OCTREE_INVALID;
    }
    if ((giff_u32)(*io_used) >= (giff_u32)GIFF_OCTREE_MAX_NODES) {
        return GIFF_OCTREE_INVALID;
    }

    index = *io_used;
    *io_used = (giff_u16)(*io_used + 1u);
    giff_octree_init_node(&nodes[index], level, is_leaf);

    if (!is_leaf && reducible_heads != 0 && level < (giff_u8)GIFF_OCTREE_MAX_DEPTH) {
        nodes[index].next_reducible = reducible_heads[level];
        reducible_heads[level] = index;
    }

    return index;
}

static giff_result giff_octree_add_color(
    giff_oct_node* nodes,
    giff_u16* io_used,
    giff_u16* io_leaf_count,
    giff_u16* reducible_heads,
    const giff_rgba8* px
)
{
    giff_u16 node_index;
    giff_u8 level;

    if (nodes == 0 || io_used == 0 || io_leaf_count == 0 || px == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    node_index = 0u;
    nodes[node_index].count += 1u;
    nodes[node_index].rsum += (giff_u32)px->r;
    nodes[node_index].gsum += (giff_u32)px->g;
    nodes[node_index].bsum += (giff_u32)px->b;

    for (level = 0u; level < (giff_u8)GIFF_OCTREE_MAX_DEPTH; ++level) {
        giff_u8 child_slot;
        giff_u16 child_index;
        giff_u8 child_is_leaf;

        child_slot = giff_octree_child_index(px, level);
        child_index = nodes[node_index].child[child_slot];
        if (child_index == GIFF_OCTREE_INVALID) {
            child_is_leaf = (giff_u8)((level + 1u == (giff_u8)GIFF_OCTREE_MAX_DEPTH) ? 1u : 0u);
            child_index = giff_octree_new_node(nodes, io_used, reducible_heads, (giff_u8)(level + 1u), child_is_leaf);
            if (child_index == GIFF_OCTREE_INVALID) {
                return GIFF_E_NO_WORKSPACE;
            }
            nodes[node_index].child[child_slot] = child_index;
            if (child_is_leaf) {
                *io_leaf_count = (giff_u16)(*io_leaf_count + 1u);
            }
        }

        node_index = child_index;
        nodes[node_index].count += 1u;
        nodes[node_index].rsum += (giff_u32)px->r;
        nodes[node_index].gsum += (giff_u32)px->g;
        nodes[node_index].bsum += (giff_u32)px->b;
    }

    return GIFF_OK;
}

static giff_u16 giff_octree_leaf_count(const giff_oct_node* nodes, giff_u16 node_index)
{
    const giff_oct_node* node;
    giff_u16 i;
    giff_u16 total;

    node = &nodes[node_index];
    if (node->is_leaf) {
        return 1u;
    }

    total = 0u;
    for (i = 0u; i < 8u; ++i) {
        if (node->child[i] != GIFF_OCTREE_INVALID) {
            total = (giff_u16)(total + giff_octree_leaf_count(nodes, node->child[i]));
        }
    }
    return total;
}

static giff_u16 giff_octree_reduce_once(giff_oct_node* nodes, giff_u16* reducible_heads, giff_u16* io_leaf_count)
{
    giff_s16 level;

    for (level = (giff_s16)GIFF_OCTREE_MAX_DEPTH - 1; level >= 0; --level) {
        while (reducible_heads[level] != GIFF_OCTREE_INVALID) {
            giff_u16 node_index;
            giff_u16 leaves;
            giff_u16 i;

            node_index = reducible_heads[level];
            reducible_heads[level] = nodes[node_index].next_reducible;
            leaves = giff_octree_leaf_count(nodes, node_index);
            if (leaves <= 1u) {
                continue;
            }

            for (i = 0u; i < 8u; ++i) {
                nodes[node_index].child[i] = GIFF_OCTREE_INVALID;
            }
            nodes[node_index].is_leaf = 1u;
            *io_leaf_count = (giff_u16)(*io_leaf_count - leaves + 1u);
            return node_index;
        }
    }

    return GIFF_OCTREE_INVALID;
}

static void giff_octree_collect_palette(
    giff_oct_node* nodes,
    giff_u16 node_index,
    giff_rgb8* out_palette,
    giff_u16 max_entries,
    giff_u16* io_entries
)
{
    giff_oct_node* node;
    giff_u16 i;

    if (nodes == 0 || out_palette == 0 || io_entries == 0) {
        return;
    }

    node = &nodes[node_index];
    if (node->count == 0u) {
        return;
    }

    if (node->is_leaf) {
        if (*io_entries >= max_entries) {
            return;
        }
        node->palette_index = *io_entries;
        out_palette[*io_entries].r = (giff_u8)((node->rsum + node->count / 2u) / node->count);
        out_palette[*io_entries].g = (giff_u8)((node->gsum + node->count / 2u) / node->count);
        out_palette[*io_entries].b = (giff_u8)((node->bsum + node->count / 2u) / node->count);
        *io_entries = (giff_u16)(*io_entries + 1u);
        return;
    }

    for (i = 0u; i < 8u; ++i) {
        if (node->child[i] != GIFF_OCTREE_INVALID) {
            giff_octree_collect_palette(nodes, node->child[i], out_palette, max_entries, io_entries);
        }
    }
}

static giff_result giff_octree_map_direct(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 palette_entries,
    giff_s32 r,
    giff_s32 g,
    giff_s32 b,
    giff_u8* out_index,
    giff_rgb8* out_color
)
{
    giff_oct_node* nodes;
    giff_u16 node_index;
    giff_u8 level;
    giff_u8 rr;
    giff_u8 gg;
    giff_u8 bb;

    if (enc == 0 || enc->oct_nodes == 0 || palette == 0 || out_index == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (!enc->octree_map_valid || enc->octree_map_entries == 0u || enc->octree_map_entries > palette_entries) {
        return GIFF_E_UNSUPPORTED;
    }

    rr = giff_clamp_u8(r);
    gg = giff_clamp_u8(g);
    bb = giff_clamp_u8(b);
    nodes = (giff_oct_node*)enc->oct_nodes;
    node_index = 0u;

    for (level = 0u; level < (giff_u8)GIFF_OCTREE_MAX_DEPTH; ++level) {
        giff_oct_node* node;
        giff_u8 child_slot;
        giff_u16 child_index;

        node = &nodes[node_index];
        if (node->is_leaf) {
            break;
        }

        child_slot = giff_octree_child_index_rgb(rr, gg, bb, level);
        child_index = node->child[child_slot];
        if (child_index == GIFF_OCTREE_INVALID) {
            break;
        }
        node_index = child_index;
    }

    if (!nodes[node_index].is_leaf ||
        nodes[node_index].palette_index == GIFF_OCTREE_INVALID ||
        nodes[node_index].palette_index >= palette_entries) {
        return GIFF_E_UNSUPPORTED;
    }

    *out_index = (giff_u8)nodes[node_index].palette_index;
    if (out_color != 0) {
        *out_color = palette[nodes[node_index].palette_index];
    }
    return GIFF_OK;
}

static giff_result giff_palette_build_from_rgba_octree(
    giff_encoder* enc,
    const giff_rgba8* pixels,
    giff_u16 width,
    giff_u16 height,
    giff_u32 stride,
    giff_rgb8* out_palette,
    giff_u16* out_entries,
    giff_u16 max_entries,
    giff_u8 reserve_transparent,
    giff_u8* out_transparent_index,
    giff_u8* out_has_transparent
)
{
    giff_oct_node* nodes;
    giff_u16 reducible_heads[GIFF_OCTREE_MAX_DEPTH];
    giff_u16 nodes_used;
    giff_u16 leaf_count;
    giff_u16 color_slots;
    giff_u16 y;
    giff_u16 x;
    giff_u16 entries;
    giff_u8 has_transparent;
    giff_u8 transparent_index;
    giff_result rc;

    if (enc == 0 || pixels == 0 || out_palette == 0 || out_entries == 0 || enc->oct_nodes == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (width == 0u || height == 0u) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    if (max_entries == 0u) {
        max_entries = 256u;
    }
    if (max_entries > 256u) {
        max_entries = 256u;
    }
    if (stride == 0u) {
        stride = (giff_u32)width * (giff_u32)sizeof(giff_rgba8);
    }

    nodes = (giff_oct_node*)enc->oct_nodes;
    giff_mem_zero(nodes, (giff_u32)(GIFF_OCTREE_MAX_NODES * (giff_u32)sizeof(giff_oct_node)));
    for (x = 0u; x < (giff_u16)GIFF_OCTREE_MAX_DEPTH; ++x) {
        reducible_heads[x] = GIFF_OCTREE_INVALID;
    }

    nodes_used = 1u;
    leaf_count = 0u;
    giff_octree_init_node(&nodes[0], 0u, 0u);
    has_transparent = 0u;

    for (y = 0u; y < height; ++y) {
        const giff_rgba8* row;
        row = (const giff_rgba8*)((const giff_u8*)pixels + (giff_u32)y * stride);
        for (x = 0u; x < width; ++x) {
            if (reserve_transparent && row[x].a < 128u) {
                has_transparent = 1u;
                continue;
            }
            rc = giff_octree_add_color(nodes, &nodes_used, &leaf_count, reducible_heads, &row[x]);
            if (rc != GIFF_OK) {
                return rc;
            }
        }
    }

    transparent_index = 0u;
    color_slots = max_entries;
    if (has_transparent && reserve_transparent && color_slots > 0u) {
        color_slots = (giff_u16)(color_slots - 1u);
    }
    if (color_slots == 0u) {
        return GIFF_E_BAD_COLOR_TABLE;
    }

    if (leaf_count == 0u && has_transparent) {
        out_palette[0].r = 0u;
        out_palette[0].g = 0u;
        out_palette[0].b = 0u;
        *out_entries = 1u;
        if (out_transparent_index != 0) {
            *out_transparent_index = 0u;
        }
        if (out_has_transparent != 0) {
            *out_has_transparent = 1u;
        }
        return GIFF_OK;
    }

    while (leaf_count > color_slots) {
        if (giff_octree_reduce_once(nodes, reducible_heads, &leaf_count) == GIFF_OCTREE_INVALID) {
            break;
        }
    }

    entries = 0u;
    if (has_transparent && reserve_transparent) {
        out_palette[0].r = 0u;
        out_palette[0].g = 0u;
        out_palette[0].b = 0u;
        transparent_index = 0u;
        entries = 1u;
    }

    giff_octree_collect_palette(nodes, 0u, out_palette, max_entries, &entries);

    if (entries == 1u && !(has_transparent && reserve_transparent)) {
        out_palette[1] = out_palette[0];
        entries = 2u;
    }
    if (entries == 0u) {
        out_palette[0].r = 0u;
        out_palette[0].g = 0u;
        out_palette[0].b = 0u;
        out_palette[1].r = 255u;
        out_palette[1].g = 255u;
        out_palette[1].b = 255u;
        entries = 2u;
    }

    *out_entries = entries;
    if (out_transparent_index != 0) {
        *out_transparent_index = transparent_index;
    }
    if (out_has_transparent != 0) {
        *out_has_transparent = has_transparent;
    }

    return GIFF_OK;
}

giff_result giff_palette_build_from_rgba(
    giff_encoder* enc,
    const giff_rgba8* pixels,
    giff_u16 width,
    giff_u16 height,
    giff_u32 stride,
    giff_rgb8* out_palette,
    giff_u16* out_entries,
    giff_u16 max_entries,
    giff_u8 reserve_transparent,
    giff_u8* out_transparent_index,
    giff_u8* out_has_transparent
)
{
    giff_u16 y;
    giff_u16 x;
    giff_u16 bins_used;
    giff_u16 color_slots;
    giff_u16 used_boxes;
    giff_u16 i;
    giff_u8 has_transparent;
    giff_u8 transparent_index;
    giff_quant_box boxes[256];
    giff_result rc;

    if (enc == 0 || pixels == 0 || out_palette == 0 || out_entries == 0 ||
        enc->quant_hist == 0 || enc->quant_bins == 0 || enc->quant_bins_tmp == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (enc->cfg.quantizer_mode == (giff_u8)GIFF_QUANTIZER_OCTREE) {
        return giff_palette_build_from_rgba_octree(
            enc,
            pixels,
            width,
            height,
            stride,
            out_palette,
            out_entries,
            max_entries,
            reserve_transparent,
            out_transparent_index,
            out_has_transparent
        );
    }

    if (width == 0u || height == 0u) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    if (max_entries == 0u) {
        max_entries = 256u;
    }
    if (max_entries > 256u) {
        max_entries = 256u;
    }

    if (stride == 0u) {
        stride = (giff_u32)width * (giff_u32)sizeof(giff_rgba8);
    }

    giff_mem_zero(enc->quant_hist, (giff_u32)(GIFF_QUANT_HIST_SIZE * sizeof(giff_u32)));
    has_transparent = 0u;

    for (y = 0u; y < height; ++y) {
        const giff_rgba8* row;
        row = (const giff_rgba8*)((const giff_u8*)pixels + (giff_u32)y * stride);
        for (x = 0u; x < width; ++x) {
            giff_u16 bin;
            if (reserve_transparent && row[x].a < 128u) {
                has_transparent = 1u;
                continue;
            }
            bin = (giff_u16)((((giff_u16)row[x].r >> 3u) << 10u) |
                             (((giff_u16)row[x].g >> 3u) << 5u) |
                             ((giff_u16)row[x].b >> 3u));
            enc->quant_hist[bin] += 1u;
        }
    }

    transparent_index = 0u;
    color_slots = max_entries;
    if (has_transparent && reserve_transparent && color_slots > 0u) {
        color_slots = (giff_u16)(color_slots - 1u);
    }
    if (color_slots == 0u) {
        return GIFF_E_BAD_COLOR_TABLE;
    }

    bins_used = 0u;
    for (i = 0u; i < (giff_u16)GIFF_QUANT_HIST_SIZE; ++i) {
        if (enc->quant_hist[i] != 0u) {
            if (bins_used >= (giff_u16)GIFF_QUANT_HIST_SIZE) {
                return GIFF_E_INTERNAL;
            }
            enc->quant_bins[bins_used++] = i;
        }
    }

    if (bins_used == 0u && has_transparent) {
        out_palette[0].r = 0u;
        out_palette[0].g = 0u;
        out_palette[0].b = 0u;
        *out_entries = 1u;
        if (out_transparent_index != 0) {
            *out_transparent_index = 0u;
        }
        if (out_has_transparent != 0) {
            *out_has_transparent = 1u;
        }
        return GIFF_OK;
    }

    for (i = 0u; i < 256u; ++i) {
        boxes[i].start = 0u;
        boxes[i].count = 0u;
        boxes[i].rmin = 0u;
        boxes[i].rmax = 0u;
        boxes[i].gmin = 0u;
        boxes[i].gmax = 0u;
        boxes[i].bmin = 0u;
        boxes[i].bmax = 0u;
        boxes[i].weight = 0u;
    }

    boxes[0].start = 0u;
    boxes[0].count = bins_used;
    giff_quant_box_recompute(enc, enc->quant_bins, &boxes[0]);
    used_boxes = 1u;

    while (used_boxes < color_slots) {
        rc = giff_quant_split_box(enc, enc->quant_bins, enc->quant_bins_tmp, boxes, &used_boxes);
        if (rc != GIFF_OK) {
            break;
        }
    }

    i = 0u;
    if (has_transparent && reserve_transparent) {
        out_palette[0].r = 0u;
        out_palette[0].g = 0u;
        out_palette[0].b = 0u;
        i = 1u;
        transparent_index = 0u;
    }

    for (x = 0u; x < used_boxes; ++x) {
        giff_quant_box_to_palette(enc, enc->quant_bins, &boxes[x], &out_palette[i]);
        i = (giff_u16)(i + 1u);
    }

    if (i == 1u && !(has_transparent && reserve_transparent)) {
        out_palette[1] = out_palette[0];
        i = 2u;
    }

    if (i == 0u) {
        out_palette[0].r = 0u;
        out_palette[0].g = 0u;
        out_palette[0].b = 0u;
        out_palette[1].r = 255u;
        out_palette[1].g = 255u;
        out_palette[1].b = 255u;
        i = 2u;
    }

    *out_entries = i;
    if (out_transparent_index != 0) {
        *out_transparent_index = transparent_index;
    }
    if (out_has_transparent != 0) {
        *out_has_transparent = has_transparent;
    }

    return GIFF_OK;
}

giff_result giff_palette_map_rgb(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 palette_entries,
    giff_u8 transparent_index,
    giff_u8 has_transparent,
    giff_s32 r,
    giff_s32 g,
    giff_s32 b,
    giff_u8 use_octree_direct,
    giff_u8* out_index,
    giff_rgb8* out_color
)
{
    giff_result rc;

    if (palette == 0 || out_index == 0 || palette_entries == 0u) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (use_octree_direct && enc != 0) {
        rc = giff_octree_map_direct(enc, palette, palette_entries, r, g, b, out_index, out_color);
        if (rc == GIFF_OK) {
            return GIFF_OK;
        }
    }

    return giff_palette_find_nearest(
        palette,
        palette_entries,
        transparent_index,
        has_transparent,
        r,
        g,
        b,
        out_index,
        out_color
    );
}

giff_result giff_palette_map_rgba(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 palette_entries,
    giff_u8 transparent_index,
    giff_u8 has_transparent,
    const giff_rgba8* px,
    giff_u16 x,
    giff_u16 y,
    giff_u8 dither_mode,
    giff_u8 use_octree_direct,
    giff_u8* out_index
)
{
    giff_s32 r;
    giff_s32 g;
    giff_s32 b;
    giff_s32 delta;

    if (palette == 0 || px == 0 || out_index == 0 || palette_entries == 0u) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (has_transparent && px->a < 128u) {
        *out_index = transparent_index;
        return GIFF_OK;
    }

    r = (giff_s32)px->r;
    g = (giff_s32)px->g;
    b = (giff_s32)px->b;

    if (dither_mode == (giff_u8)GIFF_DITHER_ORDERED4X4) {
        delta = giff_ordered_delta(x, y);
        r += delta;
        g += delta;
        b += delta;
    }

    return giff_palette_map_rgb(
        enc,
        palette,
        palette_entries,
        transparent_index,
        has_transparent,
        r,
        g,
        b,
        use_octree_direct,
        out_index,
        0
    );
}
