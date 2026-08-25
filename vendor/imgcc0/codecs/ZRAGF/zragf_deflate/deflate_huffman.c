#include "deflate_huffman.h"
#include "deflate_rle.h"
#include <string.h>

void zragf_huff_build_lengths(zragf_huff_tables *ht,
                              const zragf_block_stats *st)
{
    if (!ht || !st)
        return;
    memset(ht->ll_len, 0, sizeof(ht->ll_len));
    memset(ht->d_len, 0, sizeof(ht->d_len));
    zragf_deflate_huff_build_lengths(st->litlen_freq, 286, 15, ht->ll_len);
    zragf_deflate_huff_build_lengths(st->dist_freq, 30, 15, ht->d_len);
}

void zragf_huff_make_canonical(zragf_huff_tables *ht)
{
    zragf_deflate_huff_code tmp_ll[ZRAGF_MAX_LITLEN];
    zragf_deflate_huff_code tmp_d[ZRAGF_MAX_DIST];
    int i;

    if (!ht)
        return;

    zragf_deflate_huff_make_codes(ht->ll_len, ZRAGF_MAX_LITLEN, tmp_ll);
    zragf_deflate_huff_make_codes(ht->d_len, ZRAGF_MAX_DIST, tmp_d);
    for (i = 0; i < ZRAGF_MAX_LITLEN; ++i) {
        ht->litlen[i].code = tmp_ll[i].code;
        ht->litlen[i].len = tmp_ll[i].len;
    }
    for (i = 0; i < ZRAGF_MAX_DIST; ++i) {
        ht->dist[i].code = tmp_d[i].code;
        ht->dist[i].len = tmp_d[i].len;
    }
}

void zragf_huff_build_cl(zragf_huff_tables *ht)
{
    zragf_cl_token legacy_tokens[1024];
    zragf_deflate_cl_token shared_tokens[ZRAGF_DEFLATE_CL_TOKEN_CAP];
    unsigned freq[19];
    zragf_size_t ntok;
    int ll_count;
    int d_count;
    int i;

    if (!ht)
        return;

    memset(ht->cl_len, 0, sizeof(ht->cl_len));
    memset(freq, 0, sizeof(freq));

    zragf_deflate_huff_trim_counts(ht->ll_len, ht->d_len, &ll_count, &d_count, NULL);
    ntok = zragf_huff_rle_lengths(ht->ll_len, ll_count - 257, ht->d_len, d_count - 1, legacy_tokens, (zragf_size_t)1024u);
    if (zragf_deflate_huff_rle_code_lengths(ht->ll_len, ll_count, ht->d_len, d_count,
                                            shared_tokens, ZRAGF_DEFLATE_CL_TOKEN_CAP, &ntok)) {
        for (i = 0; i < (int)ntok; ++i) {
            if (shared_tokens[i].sym >= 0 && shared_tokens[i].sym < 19)
                freq[shared_tokens[i].sym]++;
        }
    } else {
        for (i = 0; i < (int)ntok; ++i) {
            if (legacy_tokens[i].sym >= 0 && legacy_tokens[i].sym < 19)
                freq[legacy_tokens[i].sym]++;
        }
    }
    if (ntok == 0u)
        freq[0] = 1u;

    zragf_deflate_huff_build_lengths(freq, 19, 7, ht->cl_len);
    if (ht->cl_len[0] == 0)
        ht->cl_len[0] = 1;

    {
        zragf_deflate_huff_code tmp[ZRAGF_MAX_CODELEN];
        zragf_deflate_huff_make_codes(ht->cl_len, ZRAGF_MAX_CODELEN, tmp);
        for (i = 0; i < ZRAGF_MAX_CODELEN; ++i) {
            ht->codelen[i].code = tmp[i].code;
            ht->codelen[i].len = tmp[i].len;
        }
    }
}

void zragf_huff_compute_header_sizes(zragf_huff_tables *ht)
{
    int ll_count;
    int d_count;
    int hclen_count;

    if (!ht)
        return;

    zragf_deflate_huff_trim_counts(ht->ll_len, ht->d_len, &ll_count, &d_count, &hclen_count);
    ht->hlit = ll_count - 257;
    ht->hdist = d_count - 1;
    ht->hclen = hclen_count - 4;
    if (ht->hlit < 0) ht->hlit = 0;
    if (ht->hlit > 29) ht->hlit = 29;
    if (ht->hdist < 0) ht->hdist = 0;
    if (ht->hdist > 29) ht->hdist = 29;
    if (ht->hclen < 0) ht->hclen = 0;
    if (ht->hclen > 15) ht->hclen = 15;
}
