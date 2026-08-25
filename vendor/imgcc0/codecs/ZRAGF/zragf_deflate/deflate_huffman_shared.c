#include "deflate_huffman_shared.h"
#include <string.h>

typedef struct {
    unsigned freq;
    int      symbol;
    int      parent;
} zragf_deflate_hnode;


unsigned zragf_deflate_huff_reverse_bits(unsigned code, int bits)
{
    unsigned rev = 0u;
    int i;

    for (i = 0; i < bits; ++i) {
        rev = (rev << 1) | (code & 1u);
        code >>= 1u;
    }
    return rev;
}

void zragf_deflate_huff_reverse_codes(zragf_deflate_huff_code *codes,
                                      int count)
{
    int i;
    if (!codes || count < 0)
        return;
    for (i = 0; i < count; ++i) {
        if (codes[i].len > 0)
            codes[i].code = zragf_deflate_huff_reverse_bits(codes[i].code, codes[i].len);
    }
}

static int zragf_deflate_prepare_dynamic_header(const int *ll_len,
                                                const int *d_len,
                                                int *out_hlit,
                                                int *out_hdist,
                                                int *out_hclen,
                                                int cl_len[19],
                                                zragf_deflate_huff_code cl_codes[19],
                                                zragf_deflate_cl_token *rle,
                                                zragf_size_t rle_cap,
                                                zragf_size_t *rle_count)
{
    unsigned cl_freq[19];
    int hlit = 0;
    int hdist = 0;
    int hclen = 0;
    int ll_count;
    int d_count;
    int i;

    if (!ll_len || !d_len || !out_hlit || !out_hdist || !out_hclen ||
        !cl_len || !cl_codes || !rle || !rle_count)
        return 0;

    for (i = 285; i >= 257; --i) {
        if (ll_len[i] > 0) {
            hlit = i - 256;
            break;
        }
    }
    for (i = 29; i >= 0; --i) {
        if (d_len[i] > 0) {
            hdist = i;
            break;
        }
    }

    ll_count = 257 + hlit;
    d_count = 1 + hdist;

    if (!zragf_deflate_huff_rle_code_lengths(ll_len, ll_count, d_len, d_count,
                                             rle, rle_cap, rle_count))
        return 0;

    memset(cl_freq, 0, sizeof(cl_freq));
    for (i = 0; i < (int)(*rle_count); ++i) {
        int sym = rle[i].sym;
        if (sym < 0 || sym >= 19)
            return 0;
        cl_freq[sym]++;
    }
    if (*rle_count == 0u)
        cl_freq[0] = 1u;

    zragf_deflate_huff_build_lengths(cl_freq, 19, 7, cl_len);
    if (zragf_deflate_huff_lengths_oversubscribed(cl_len, 19, 7))
        return 0;
    zragf_deflate_huff_make_codes(cl_len, 19, cl_codes);
    zragf_deflate_huff_reverse_codes(cl_codes, 19);

    hclen = 0;
    for (i = 18; i >= 4; --i) {
        if (cl_len[zragf_deflate_cl_order[i]] > 0) {
            hclen = i - 3;
            break;
        }
    }

    *out_hlit = hlit;
    *out_hdist = hdist;
    *out_hclen = hclen;
    return 1;
}

int zragf_deflate_prepare_dynamic(const zragf_block_stats *stats,
                                  zragf_deflate_dynamic_prepared *out)
{
    unsigned ll_freq[286];
    unsigned d_freq[30];
    int i;
    int any_dist = 0;

    if (!stats || !out)
        return 0;

    memset(out, 0, sizeof(*out));
    memcpy(ll_freq, stats->litlen_freq, sizeof(ll_freq));
    memcpy(d_freq, stats->dist_freq, sizeof(d_freq));
    if (ll_freq[256] == 0u)
        ll_freq[256] = 1u;

    zragf_deflate_huff_build_lengths(ll_freq, 286, 15, out->ll_len);
    zragf_deflate_huff_build_lengths(d_freq, 30, 15, out->d_len);

    for (i = 0; i < 30; ++i) {
        if (d_freq[i] > 0u) {
            any_dist = 1;
            break;
        }
    }
    if (!any_dist)
        out->d_len[0] = 1;

    if (zragf_deflate_huff_lengths_oversubscribed(out->ll_len, 286, 15) ||
        zragf_deflate_huff_lengths_oversubscribed(out->d_len, 30, 15))
        return 0;

    zragf_deflate_huff_make_codes(out->ll_len, 286, out->ll_codes);
    zragf_deflate_huff_make_codes(out->d_len, 30, out->d_codes);
    zragf_deflate_huff_reverse_codes(out->ll_codes, 286);
    zragf_deflate_huff_reverse_codes(out->d_codes, 30);

    if (!zragf_deflate_prepare_dynamic_header(out->ll_len, out->d_len,
                                              &out->hlit, &out->hdist, &out->hclen,
                                              out->cl_len, out->cl_codes,
                                              out->rle, ZRAGF_DEFLATE_CL_TOKEN_CAP, &out->rle_count))
        return 0;

    out->header_bits = (unsigned)(5 + 5 + 4 + (3 * (4 + out->hclen)));
    for (i = 0; i < (int)out->rle_count; ++i) {
        int sym = out->rle[i].sym;
        if (sym < 0 || sym >= 19)
            return 0;
        out->header_bits += (unsigned)out->cl_len[sym];
        out->header_bits += (unsigned)out->rle[i].extra_bits;
    }

    out->valid = 1;
    return 1;
}

const int zragf_deflate_cl_order[19] = {
    16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
};

void zragf_deflate_huff_build_lengths(const unsigned *freq,
                                      int count,
                                      int maxbits,
                                      int *out_len)
{
    zragf_deflate_hnode nodes[600];
    int i;
    int n = 0;
    int total;

    if (!freq || !out_len || count <= 0 || maxbits <= 0)
        return;

    for (i = 0; i < count; ++i)
        out_len[i] = 0;

    for (i = 0; i < count; ++i) {
        if (freq[i] > 0u) {
            nodes[n].freq = freq[i];
            nodes[n].symbol = i;
            nodes[n].parent = -1;
            n++;
        }
    }

    if (n == 0)
        return;
    if (n == 1) {
        out_len[nodes[0].symbol] = 1;
        return;
    }

    total = n;
    while (1) {
        int m1 = -1;
        int m2 = -1;
        for (i = 0; i < total; ++i) {
            if (nodes[i].parent >= 0)
                continue;
            if (m1 < 0 || nodes[i].freq < nodes[m1].freq ||
                (nodes[i].freq == nodes[m1].freq && nodes[i].symbol < nodes[m1].symbol)) {
                m2 = m1;
                m1 = i;
            } else if (m2 < 0 || nodes[i].freq < nodes[m2].freq ||
                       (nodes[i].freq == nodes[m2].freq && nodes[i].symbol < nodes[m2].symbol)) {
                m2 = i;
            }
        }
        if (m2 < 0)
            break;
        nodes[total].freq = nodes[m1].freq + nodes[m2].freq;
        nodes[total].symbol = -1;
        nodes[total].parent = -1;
        nodes[m1].parent = total;
        nodes[m2].parent = total;
        total++;
    }

    for (i = 0; i < total; ++i) {
        if (nodes[i].symbol >= 0) {
            int d = 0;
            int p = nodes[i].parent;
            while (p >= 0) {
                d++;
                p = nodes[p].parent;
            }
            if (d > maxbits)
                d = maxbits;
            out_len[nodes[i].symbol] = d;
        }
    }
}

int zragf_deflate_huff_lengths_oversubscribed(const int *lens,
                                              int count,
                                              int maxbits)
{
    int bl_count[32];
    int left = 1;
    int i;
    int bits;

    if (!lens || count < 0 || maxbits <= 0)
        return 1;

    memset(bl_count, 0, sizeof(bl_count));
    for (i = 0; i < count; ++i) {
        if (lens[i] < 0 || lens[i] > maxbits)
            return 1;
        if (lens[i] > 0)
            bl_count[lens[i]]++;
    }

    for (bits = 1; bits <= maxbits; ++bits) {
        left <<= 1;
        left -= bl_count[bits];
        if (left < 0)
            return 1;
    }
    return 0;
}

void zragf_deflate_huff_make_codes(const int *lens,
                                   int count,
                                   zragf_deflate_huff_code *out)
{
    int bl_count[32];
    int next_code[32];
    unsigned code = 0u;
    int i;
    int bits;

    if (!lens || !out || count < 0)
        return;

    memset(bl_count, 0, sizeof(bl_count));
    memset(next_code, 0, sizeof(next_code));

    for (i = 0; i < count; ++i) {
        if (lens[i] > 0)
            bl_count[lens[i]]++;
    }

    for (bits = 1; bits <= 15; ++bits) {
        code = (code + (unsigned)bl_count[bits - 1]) << 1;
        next_code[bits] = (int)code;
    }

    for (i = 0; i < count; ++i) {
        out[i].len = lens[i];
        if (lens[i] > 0)
            out[i].code = (unsigned)next_code[lens[i]]++;
        else
            out[i].code = 0u;
    }
}

int zragf_deflate_huff_rle_code_lengths(const int *ll_len,
                                        int ll_count,
                                        const int *d_len,
                                        int d_count,
                                        zragf_deflate_cl_token *out,
                                        zragf_size_t out_cap,
                                        zragf_size_t *out_count)
{
    int total;
    int i;

    if (!ll_len || !d_len || !out || !out_count)
        return 0;

    total = ll_count + d_count;
    i = 0;
    *out_count = 0u;

    while (i < total) {
        int cur_len;
        int run = 1;
        int j;

        if (i < ll_count)
            cur_len = ll_len[i];
        else
            cur_len = d_len[i - ll_count];

        for (j = i + 1; j < total; ++j) {
            int next_len;
            if (j < ll_count)
                next_len = ll_len[j];
            else
                next_len = d_len[j - ll_count];
            if (next_len != cur_len)
                break;
            run++;
        }

        if (cur_len == 0) {
            while (run > 0) {
                zragf_deflate_cl_token tok;
                if (run >= 11) {
                    int count = (run > 138) ? 138 : run;
                    tok.sym = 18;
                    tok.extra_bits = 7;
                    tok.extra_val = (unsigned)(count - 11);
                    run -= count;
                } else if (run >= 3) {
                    int count = (run > 10) ? 10 : run;
                    tok.sym = 17;
                    tok.extra_bits = 3;
                    tok.extra_val = (unsigned)(count - 3);
                    run -= count;
                } else {
                    tok.sym = 0;
                    tok.extra_bits = 0;
                    tok.extra_val = 0u;
                    run--;
                }
                if (*out_count >= out_cap)
                    return 0;
                out[(*out_count)++] = tok;
            }
        } else {
            int first = 1;
            while (run > 0) {
                zragf_deflate_cl_token tok;
                if (first) {
                    tok.sym = cur_len;
                    tok.extra_bits = 0;
                    tok.extra_val = 0u;
                    first = 0;
                    run--;
                } else if (run >= 3) {
                    int count = (run > 6) ? 6 : run;
                    tok.sym = 16;
                    tok.extra_bits = 2;
                    tok.extra_val = (unsigned)(count - 3);
                    run -= count;
                } else {
                    tok.sym = cur_len;
                    tok.extra_bits = 0;
                    tok.extra_val = 0u;
                    run--;
                }
                if (*out_count >= out_cap)
                    return 0;
                out[(*out_count)++] = tok;
            }
        }

        i = j;
    }

    return 1;
}

void zragf_deflate_huff_trim_counts(const int *ll_len,
                                    const int *d_len,
                                    int *out_ll_count,
                                    int *out_d_count,
                                    int *out_hclen_count)
{
    int ll_count = 257;
    int d_count = 1;
    int cl_count = 4;
    int i;
    int cl_len[19];
    unsigned cl_freq[19];
    zragf_deflate_cl_token toks[ZRAGF_DEFLATE_CL_TOKEN_CAP];
    zragf_size_t ntok = 0u;

    if (out_ll_count)
        *out_ll_count = 257;
    if (out_d_count)
        *out_d_count = 1;
    if (out_hclen_count)
        *out_hclen_count = 4;
    if (!ll_len || !d_len)
        return;

    for (i = 285; i >= 257; --i) {
        if (ll_len[i] > 0) {
            ll_count = i + 1;
            break;
        }
    }
    for (i = 29; i >= 0; --i) {
        if (d_len[i] > 0) {
            d_count = i + 1;
            break;
        }
    }

    if (out_ll_count)
        *out_ll_count = ll_count;
    if (out_d_count)
        *out_d_count = d_count;

    memset(cl_len, 0, sizeof(cl_len));
    memset(cl_freq, 0, sizeof(cl_freq));
    if (zragf_deflate_huff_rle_code_lengths(ll_len, ll_count, d_len, d_count,
                                            toks, ZRAGF_DEFLATE_CL_TOKEN_CAP, &ntok)) {
        for (i = 0; i < (int)ntok; ++i) {
            if (toks[i].sym >= 0 && toks[i].sym < 19)
                cl_freq[toks[i].sym]++;
        }
    }
    if (ntok == 0u)
        cl_freq[0] = 1u;

    zragf_deflate_huff_build_lengths(cl_freq, 19, 7, cl_len);
    if (cl_len[0] == 0)
        cl_len[0] = 1;

    for (i = 18; i >= 4; --i) {
        if (cl_len[zragf_deflate_cl_order[i]] > 0) {
            cl_count = i + 1;
            break;
        }
    }
    if (out_hclen_count)
        *out_hclen_count = cl_count;
}
