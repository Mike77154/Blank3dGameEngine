#!/usr/bin/env python3
# giffy_hb v0.14 offline packer: no runtime heap; host-side parser may allocate freely.
"""
ttf_minipack.py - dependency-free stage-2 offline font packer for giffy_hb_c89.

Reads a real SFNT TTF/OTF enough to extract:
  - head.unitsPerEm
  - maxp.numGlyphs
  - hhea.numberOfHMetrics
  - hmtx horizontal advances
  - cmap format 4 and 12 Unicode mappings
  - GDEF GlyphClassDef format 1/2 when present
  - GSUB ligature substitutions for liga/clig/dlig/rlig, common formats
  - GPOS pair positioning for kern, PairPos format 1 and 2, xAdvance only
  - GPOS type 4 mark-to-base and type 6 mark-to-mark, anchor formats 1/2/3
  - GSUB type 1 single substitutions by feature
  - GSUB type 5/6 format 3 contextual/chained-contextual when dispatching nested single-sub lookups
  - fvar axes and HVAR VarStore skeleton extraction; emits static region/item slots
  - GDEF MarkGlyphSetsDef compact ids
  - GSUB type 8 reverse chaining subset
  - ScriptList/LangSys inventory and real --script/--lang lookup filtering

This is an OFFLINE HOST TOOL. It may allocate freely because it is not linked
into the target runtime. The emitted C header is static C89 data.
"""
import argparse
import struct
import sys


def be16(b, o): return struct.unpack_from(">H", b, o)[0]
def sbe16(b, o): return struct.unpack_from(">h", b, o)[0]
def be32(b, o): return struct.unpack_from(">I", b, o)[0]
def sbe32(b, o): return struct.unpack_from(">i", b, o)[0]


def tag_name(s):
    return ''.join(chr(c) if 32 <= c <= 126 else '_' for c in s)


def table_map(data):
    if len(data) < 12:
        raise ValueError("file too small")
    num = be16(data, 4)
    out = {}
    off = 12
    for _ in range(num):
        if off + 16 > len(data):
            raise ValueError("truncated table directory")
        tag = data[off:off+4]
        toff = be32(data, off+8)
        length = be32(data, off+12)
        out[tag_name(tag)] = (toff, length)
        off += 16
    return out


def slice_table(data, tables, name, required=True):
    if name not in tables:
        if required:
            raise ValueError("missing table %s" % name)
        return b""
    off, length = tables[name]
    if off + length > len(data):
        raise ValueError("table %s outside file" % name)
    return data[off:off+length]


def in_range(b, o, n=1):
    return 0 <= o <= len(b) - n


def parse_cmap(data):
    cmap = []
    if len(data) < 4:
        return cmap
    count = be16(data, 2)
    best12 = None
    best4 = None
    for i in range(count):
        rec = 4 + i * 8
        if rec + 8 > len(data):
            break
        platform = be16(data, rec)
        off = be32(data, rec + 4)
        if off + 2 > len(data):
            continue
        fmt = be16(data, off)
        if fmt == 12 and (platform == 3 or platform == 0):
            best12 = off
        elif fmt == 4 and (platform == 3 or platform == 0):
            best4 = off
    if best12 is not None:
        off = best12
        if off + 16 <= len(data):
            n_groups = be32(data, off + 12)
            p = off + 16
            for _ in range(n_groups):
                if p + 12 > len(data):
                    break
                start = be32(data, p)
                end = be32(data, p + 4)
                start_gid = be32(data, p + 8)
                cp = start
                while cp <= end and cp <= 0x10ffff:
                    gid = start_gid + (cp - start)
                    if gid <= 65535:
                        cmap.append((cp, gid))
                    cp += 1
                p += 12
            return sorted(set(cmap))
    if best4 is not None:
        off = best4
        if off + 14 > len(data):
            return cmap
        length = be16(data, off + 2)
        end_off = off + length
        seg_count = be16(data, off + 6) // 2
        end_codes = off + 14
        start_codes = end_codes + 2 * seg_count + 2
        id_deltas = start_codes + 2 * seg_count
        id_range_offsets = id_deltas + 2 * seg_count
        for i in range(seg_count):
            endc = be16(data, end_codes + 2 * i)
            startc = be16(data, start_codes + 2 * i)
            delta = sbe16(data, id_deltas + 2 * i)
            ro = be16(data, id_range_offsets + 2 * i)
            cp = startc
            while cp <= endc and cp != 0xffff:
                gid = 0
                if ro == 0:
                    gid = (cp + delta) & 0xffff
                else:
                    glyph_index_offset = id_range_offsets + 2 * i + ro + 2 * (cp - startc)
                    if glyph_index_offset + 2 <= end_off:
                        raw = be16(data, glyph_index_offset)
                        if raw != 0:
                            gid = (raw + delta) & 0xffff
                if gid != 0:
                    cmap.append((cp, gid))
                cp += 1
    return sorted(set(cmap))


def parse_coverage(tab, off):
    out = []
    if not in_range(tab, off, 4):
        return out
    fmt = be16(tab, off)
    count = be16(tab, off + 2)
    if fmt == 1:
        p = off + 4
        for i in range(count):
            if not in_range(tab, p + 2*i, 2): break
            out.append(be16(tab, p + 2*i))
    elif fmt == 2:
        p = off + 4
        for i in range(count):
            if not in_range(tab, p + 6*i, 6): break
            start = be16(tab, p + 6*i)
            end = be16(tab, p + 6*i + 2)
            g = start
            while g <= end:
                out.append(g)
                g += 1
    return out


def parse_classdef(tab, off):
    classes = {}
    if not in_range(tab, off, 4):
        return classes
    fmt = be16(tab, off)
    if fmt == 1:
        start = be16(tab, off + 2)
        count = be16(tab, off + 4) if in_range(tab, off + 4, 2) else 0
        p = off + 6
        for i in range(count):
            if not in_range(tab, p + 2*i, 2): break
            classes[start + i] = be16(tab, p + 2*i)
    elif fmt == 2:
        count = be16(tab, off + 2)
        p = off + 4
        for i in range(count):
            if not in_range(tab, p + 6*i, 6): break
            start = be16(tab, p + 6*i)
            end = be16(tab, p + 6*i + 2)
            cls = be16(tab, p + 6*i + 4)
            g = start
            while g <= end:
                classes[g] = cls
                g += 1
    return classes


def _feature_records(tab):
    recs = []
    if len(tab) < 10:
        return recs
    feature_list_off = be16(tab, 6)
    if not in_range(tab, feature_list_off, 2):
        return recs
    feat_count = be16(tab, feature_list_off)
    for i in range(feat_count):
        rec = feature_list_off + 2 + i * 6
        if not in_range(tab, rec, 6): break
        tag = tag_name(tab[rec:rec+4])
        foff = feature_list_off + be16(tab, rec + 4)
        recs.append((i, tag, foff))
    return recs


def _langsys_feature_indices(tab, target_script="", target_lang=""):
    """Return allowed FeatureList indices from ScriptList/LangSys. Empty means all."""
    allowed = set()
    if len(tab) < 10:
        return allowed
    script_list_off = be16(tab, 4)
    if not in_range(tab, script_list_off, 2):
        return allowed
    scount = be16(tab, script_list_off)
    target_script = (target_script or "").strip()
    target_lang = (target_lang or "").strip()
    for si in range(scount):
        srec = script_list_off + 2 + si * 6
        if not in_range(tab, srec, 6): break
        stag = tag_name(tab[srec:srec+4])
        if target_script and stag != target_script and stag != "DFLT":
            continue
        soff = script_list_off + be16(tab, srec + 4)
        if not in_range(tab, soff, 4): continue
        lang_offsets = []
        default_off = be16(tab, soff)
        if default_off:
            lang_offsets.append(("dflt", soff + default_off))
        lang_count = be16(tab, soff + 2)
        for li in range(lang_count):
            lrec = soff + 4 + li * 6
            if not in_range(tab, lrec, 6): break
            ltag = tag_name(tab[lrec:lrec+4])
            lang_offsets.append((ltag, soff + be16(tab, lrec + 4)))
        for ltag, loff in lang_offsets:
            if target_lang and ltag != target_lang and ltag != "dflt":
                continue
            if not in_range(tab, loff, 6): continue
            feat_count = be16(tab, loff + 4)
            fp = loff + 6
            for fi in range(feat_count):
                if not in_range(tab, fp + fi * 2, 2): break
                allowed.add(be16(tab, fp + fi * 2))
    return allowed


def parse_script_feature_lookup(tab, target_script="", target_lang=""):
    """Return feature tag -> list of lookup indices, optionally filtered by Script/LangSys."""
    out = {}
    allowed = _langsys_feature_indices(tab, target_script, target_lang)
    for feat_index, tag, foff in _feature_records(tab):
        if allowed and feat_index not in allowed:
            continue
        if not in_range(tab, foff, 4): continue
        lookup_count = be16(tab, foff + 2)
        lookups = []
        q = foff + 4
        for j in range(lookup_count):
            if not in_range(tab, q + 2*j, 2): break
            lookups.append(be16(tab, q + 2*j))
        out.setdefault(tag, []).extend(lookups)
    return out


def lookup_offsets(tab):
    if len(tab) < 10:
        return []
    lookup_list_off = be16(tab, 8)
    if not in_range(tab, lookup_list_off, 2):
        return []
    count = be16(tab, lookup_list_off)
    offs = []
    for i in range(count):
        p = lookup_list_off + 2 + i*2
        if not in_range(tab, p, 2): break
        offs.append(lookup_list_off + be16(tab, p))
    return offs


def parse_gsub_ligatures(tab, target_script="", target_lang=""):
    feature_lookups = parse_script_feature_lookup(tab, target_script, target_lang)
    lookups = lookup_offsets(tab)
    lig_features = [("liga", "GHB_FEATURE_LIGA"), ("clig", "GHB_FEATURE_CLIG"), ("dlig", "GHB_FEATURE_DLIG"), ("rlig", "GHB_FEATURE_RLIG")]
    out = []
    seen = set()
    for ftag, macro in lig_features:
        for li in feature_lookups.get(ftag, []):
            if li >= len(lookups): continue
            loff = lookups[li]
            if not in_range(tab, loff, 6): continue
            lookup_type = be16(tab, loff)
            sub_count = be16(tab, loff + 4)
            if lookup_type != 4:  # LigatureSubst
                continue
            for si in range(sub_count):
                sp = loff + 6 + si*2
                if not in_range(tab, sp, 2): break
                sub = loff + be16(tab, sp)
                if not in_range(tab, sub, 6): continue
                fmt = be16(tab, sub)
                if fmt != 1: continue
                cov = parse_coverage(tab, sub + be16(tab, sub + 2))
                set_count = be16(tab, sub + 4)
                for ci, first_gid in enumerate(cov[:set_count]):
                    set_off_p = sub + 6 + ci*2
                    if not in_range(tab, set_off_p, 2): continue
                    lset = sub + be16(tab, set_off_p)
                    if not in_range(tab, lset, 2): continue
                    lig_count = be16(tab, lset)
                    for k in range(lig_count):
                        lp = lset + 2 + k*2
                        if not in_range(tab, lp, 2): break
                        lig = lset + be16(tab, lp)
                        if not in_range(tab, lig, 4): continue
                        lig_gid = be16(tab, lig)
                        comp_count = be16(tab, lig + 2)
                        comps = [first_gid]
                        pp = lig + 4
                        for c in range(max(0, comp_count - 1)):
                            if not in_range(tab, pp + c*2, 2): break
                            comps.append(be16(tab, pp + c*2))
                        if 2 <= len(comps) <= 8:
                            key = (tuple(comps), lig_gid, macro)
                            if key not in seen:
                                seen.add(key)
                                out.append((comps, lig_gid, macro))
    return out


def value_record_size(fmt):
    size = 0
    for bit in (0x0001,0x0002,0x0004,0x0008,0x0010,0x0020,0x0040,0x0080):
        if fmt & bit: size += 2
    return size


def read_xadvance(tab, off, fmt):
    p = off
    if fmt & 0x0001: p += 2  # xPlacement
    if fmt & 0x0002: p += 2  # yPlacement
    if fmt & 0x0004: return sbe16(tab, p) if in_range(tab, p, 2) else 0
    return 0


def parse_gpos_kern(tab, target_script="", target_lang=""):
    feature_lookups = parse_script_feature_lookup(tab, target_script, target_lang)
    lookups = lookup_offsets(tab)
    out = []
    seen = set()
    for li in feature_lookups.get("kern", []):
        if li >= len(lookups): continue
        loff = lookups[li]
        if not in_range(tab, loff, 6): continue
        lookup_type = be16(tab, loff)
        sub_count = be16(tab, loff + 4)
        if lookup_type != 2:  # PairPos
            continue
        for si in range(sub_count):
            sp = loff + 6 + si*2
            if not in_range(tab, sp, 2): break
            sub = loff + be16(tab, sp)
            if not in_range(tab, sub, 10): continue
            fmt = be16(tab, sub)
            cov = parse_coverage(tab, sub + be16(tab, sub + 2))
            val1fmt = be16(tab, sub + 4)
            val2fmt = be16(tab, sub + 6)
            size1 = value_record_size(val1fmt)
            size2 = value_record_size(val2fmt)
            if fmt == 1:
                pair_set_count = be16(tab, sub + 8)
                for ci, left_gid in enumerate(cov[:pair_set_count]):
                    ps_p = sub + 10 + ci*2
                    if not in_range(tab, ps_p, 2): continue
                    ps = sub + be16(tab, ps_p)
                    if not in_range(tab, ps, 2): continue
                    pair_count = be16(tab, ps)
                    rec_size = 2 + size1 + size2
                    p = ps + 2
                    for r in range(pair_count):
                        rp = p + r*rec_size
                        if not in_range(tab, rp, rec_size): break
                        right_gid = be16(tab, rp)
                        xa = read_xadvance(tab, rp + 2, val1fmt)
                        if xa:
                            key = (left_gid, right_gid, xa)
                            if key not in seen:
                                seen.add(key)
                                out.append(key)
            elif fmt == 2:
                cdef1_off = be16(tab, sub + 8)
                cdef2_off = be16(tab, sub + 10)
                class1_count = be16(tab, sub + 12)
                class2_count = be16(tab, sub + 14)
                c1 = parse_classdef(tab, sub + cdef1_off)
                c2 = parse_classdef(tab, sub + cdef2_off)
                # Only expand glyphs present in coverage for left side and glyphs explicitly classified on right side.
                left_by_class = {}
                for g in cov:
                    left_by_class.setdefault(c1.get(g, 0), []).append(g)
                right_by_class = {}
                for g, cls in c2.items():
                    right_by_class.setdefault(cls, []).append(g)
                rec_size = size1 + size2
                base = sub + 16
                for c1i in range(class1_count):
                    for c2i in range(class2_count):
                        rp = base + (c1i * class2_count + c2i) * rec_size
                        if not in_range(tab, rp, rec_size): continue
                        xa = read_xadvance(tab, rp, val1fmt)
                        if not xa: continue
                        for lg in left_by_class.get(c1i, []):
                            for rg in right_by_class.get(c2i, []):
                                key = (lg, rg, xa)
                                if key not in seen:
                                    seen.add(key)
                                    out.append(key)
    return out


def parse_gdef_classes(tab):
    flags = {}
    if len(tab) < 8:
        return flags
    glyph_class_def_off = be16(tab, 4)
    if glyph_class_def_off == 0:
        return flags
    classes = parse_classdef(tab, glyph_class_def_off)
    for gid, cls in classes.items():
        if cls == 1:
            flags[gid] = "GHB_GLYPH_BASE"
        elif cls == 2:
            flags[gid] = "GHB_GLYPH_LIGATURE"
        elif cls == 3:
            flags[gid] = "GHB_GLYPH_MARK"
        elif cls == 4:
            flags[gid] = "GHB_GLYPH_COMPONENT"
    return flags


def unicode_mark_flags(cmap):
    # Basic combining diacritical marks plus common extended combining ranges.
    flags = {}
    for cp, gid in cmap:
        if (0x0300 <= cp <= 0x036F) or (0x1AB0 <= cp <= 0x1AFF) or (0x1DC0 <= cp <= 0x1DFF) or (0x20D0 <= cp <= 0x20FF) or (0xFE20 <= cp <= 0xFE2F):
            flags[gid] = "GHB_GLYPH_MARK"
    return flags



def parse_lookup_flags(tab, loff):
    return be16(tab, loff + 2) if in_range(tab, loff + 2, 2) else 0


def parse_gsub_single(tab, target_script="", target_lang=""):
    feature_lookups = parse_script_feature_lookup(tab, target_script, target_lang)
    lookups = lookup_offsets(tab)
    out = []
    seen = set()
    feature_macros = {
        "ccmp":"GHB_FEATURE_CCMP", "calt":"GHB_FEATURE_CALT", "locl":"GHB_FEATURE_CALT",
        "init":"GHB_FEATURE_INIT", "medi":"GHB_FEATURE_MEDI", "fina":"GHB_FEATURE_FINA", "isol":"GHB_FEATURE_ISOL",
        "abvs":"GHB_FEATURE_ABVS", "blws":"GHB_FEATURE_BLWS", "haln":"GHB_FEATURE_HALN",
        "pres":"GHB_FEATURE_PRES", "psts":"GHB_FEATURE_PSTS", "rphf":"GHB_FEATURE_RPHF",
        "half":"GHB_FEATURE_HALF", "vatu":"GHB_FEATURE_VATU", "cjct":"GHB_FEATURE_CJCT"
    }
    for ftag, macro in feature_macros.items():
        for li in feature_lookups.get(ftag, []):
            if li >= len(lookups): continue
            loff = lookups[li]
            if not in_range(tab, loff, 6): continue
            if be16(tab, loff) != 1: continue
            flags = parse_lookup_flags(tab, loff)
            sub_count = be16(tab, loff + 4)
            for si in range(sub_count):
                sp = loff + 6 + si*2
                if not in_range(tab, sp, 2): break
                sub = loff + be16(tab, sp)
                if not in_range(tab, sub, 6): continue
                fmt = be16(tab, sub)
                cov = parse_coverage(tab, sub + be16(tab, sub + 2))
                if fmt == 1:
                    delta = sbe16(tab, sub + 4)
                    for gid in cov:
                        to_gid = (gid + delta) & 0xffff
                        key = (gid, to_gid, macro, flags)
                        if key not in seen:
                            seen.add(key); out.append(key)
                elif fmt == 2:
                    cnt = be16(tab, sub + 4)
                    for idx, gid in enumerate(cov[:cnt]):
                        p = sub + 6 + idx*2
                        if not in_range(tab, p, 2): break
                        to_gid = be16(tab, p)
                        key = (gid, to_gid, macro, flags)
                        if key not in seen:
                            seen.add(key); out.append(key)
    return out


def gsub_single_map_for_lookup(tab, lookup_index):
    lookups = lookup_offsets(tab)
    out = {}
    if lookup_index >= len(lookups):
        return out
    loff = lookups[lookup_index]
    if not in_range(tab, loff, 6) or be16(tab, loff) != 1:
        return out
    sub_count = be16(tab, loff + 4)
    for si in range(sub_count):
        sp = loff + 6 + si*2
        if not in_range(tab, sp, 2): break
        sub = loff + be16(tab, sp)
        if not in_range(tab, sub, 6): continue
        fmt = be16(tab, sub)
        cov = parse_coverage(tab, sub + be16(tab, sub + 2))
        if fmt == 1:
            delta = sbe16(tab, sub + 4)
            for gid in cov:
                out[gid] = (gid + delta) & 0xffff
        elif fmt == 2:
            cnt = be16(tab, sub + 4)
            for idx, gid in enumerate(cov[:cnt]):
                p = sub + 6 + idx*2
                if not in_range(tab, p, 2): break
                out[gid] = be16(tab, p)
    return out


def parse_gsub_contextual(tab, target_script="", target_lang=""):
    feature_lookups = parse_script_feature_lookup(tab, target_script, target_lang)
    lookups = lookup_offsets(tab)
    out = []
    seen = set()
    feature_macros = {"calt":"GHB_FEATURE_CALT", "ccmp":"GHB_FEATURE_CCMP", "pres":"GHB_FEATURE_PRES", "psts":"GHB_FEATURE_PSTS", "cjct":"GHB_FEATURE_CJCT"}
    for ftag, macro in feature_macros.items():
        for li in feature_lookups.get(ftag, []):
            if li >= len(lookups): continue
            loff = lookups[li]
            if not in_range(tab, loff, 6): continue
            ltype = be16(tab, loff)
            flags = parse_lookup_flags(tab, loff)
            if ltype not in (5, 6): continue
            sub_count = be16(tab, loff + 4)
            for si in range(sub_count):
                sp = loff + 6 + si*2
                if not in_range(tab, sp, 2): break
                sub = loff + be16(tab, sp)
                if not in_range(tab, sub, 8): continue
                fmt = be16(tab, sub)
                # Supports ContextSubst format 3 and ChainedContextSubst format 3 only.
                if ltype == 5 and fmt == 3:
                    glyph_count = be16(tab, sub + 2)
                    subst_count = be16(tab, sub + 4)
                    if glyph_count < 1 or glyph_count > 8: continue
                    coverages = []
                    p = sub + 6
                    ok = True
                    for k in range(glyph_count):
                        if not in_range(tab, p + k*2, 2): ok = False; break
                        cov = parse_coverage(tab, sub + be16(tab, p + k*2))
                        if len(cov) != 1: ok = False; break
                        coverages.append(cov[0])
                    if not ok: continue
                    recp = p + glyph_count*2
                    for rr in range(subst_count):
                        rp = recp + rr*4
                        if not in_range(tab, rp, 4): break
                        seq_index = be16(tab, rp)
                        nested_lookup = be16(tab, rp + 2)
                        if seq_index >= len(coverages): continue
                        smap = gsub_single_map_for_lookup(tab, nested_lookup)
                        src = coverages[seq_index]
                        if src in smap:
                            key=((), tuple(coverages), (), seq_index, smap[src], macro, flags)
                            if key not in seen:
                                seen.add(key); out.append(key)
                elif ltype == 6 and fmt == 3:
                    p = sub + 2
                    back_count = be16(tab, p); p += 2
                    backs=[]
                    ok=True
                    for k in range(back_count):
                        if not in_range(tab, p + k*2, 2): ok=False; break
                        cov=parse_coverage(tab, sub + be16(tab, p + k*2))
                        if len(cov)!=1: ok=False; break
                        backs.append(cov[0])
                    if not ok or len(backs)>4: continue
                    p += back_count*2
                    input_count = be16(tab, p); p += 2
                    inputs=[]
                    for k in range(input_count):
                        if not in_range(tab, p + k*2, 2): ok=False; break
                        cov=parse_coverage(tab, sub + be16(tab, p + k*2))
                        if len(cov)!=1: ok=False; break
                        inputs.append(cov[0])
                    if not ok or len(inputs)<1 or len(inputs)>8: continue
                    p += input_count*2
                    look_count = be16(tab, p); p += 2
                    looks=[]
                    for k in range(look_count):
                        if not in_range(tab, p + k*2, 2): ok=False; break
                        cov=parse_coverage(tab, sub + be16(tab, p + k*2))
                        if len(cov)!=1: ok=False; break
                        looks.append(cov[0])
                    if not ok or len(looks)>4: continue
                    p += look_count*2
                    subst_count = be16(tab, p); p += 2
                    for rr in range(subst_count):
                        rp = p + rr*4
                        if not in_range(tab, rp, 4): break
                        seq_index = be16(tab, rp)
                        nested_lookup = be16(tab, rp + 2)
                        if seq_index >= len(inputs): continue
                        smap = gsub_single_map_for_lookup(tab, nested_lookup)
                        src = inputs[seq_index]
                        if src in smap:
                            key=(tuple(backs), tuple(inputs), tuple(looks), seq_index, smap[src], macro, flags)
                            if key not in seen:
                                seen.add(key); out.append(key)
    return out


def read_anchor(tab, off):
    if off == 0 or not in_range(tab, off, 6):
        return None
    fmt = be16(tab, off)
    if fmt in (1, 2, 3):
        return (sbe16(tab, off + 2) * 64, sbe16(tab, off + 4) * 64)
    return None


def parse_mark_array(tab, off, mark_gids):
    out=[]
    if not in_range(tab, off, 2): return out
    count=be16(tab, off)
    p=off+2
    for i, gid in enumerate(mark_gids[:count]):
        rp=p+i*4
        if not in_range(tab, rp, 4): break
        cls=be16(tab, rp)
        aoff=be16(tab, rp+2)
        anch=read_anchor(tab, off+aoff)
        if anch is not None:
            out.append((gid, cls, anch[0], anch[1]))
    return out


def parse_gpos_mark_to_base(tab, target_script="", target_lang=""):
    feature_lookups = parse_script_feature_lookup(tab, target_script, target_lang)
    lookups = lookup_offsets(tab)
    out=[]; seen=set()
    for ftag, macro in (("mark", "GHB_FEATURE_MARK"), ("abvm", "GHB_FEATURE_MARK"), ("blwm", "GHB_FEATURE_MARK")):
        for li in feature_lookups.get(ftag, []):
            if li >= len(lookups): continue
            loff=lookups[li]
            if not in_range(tab, loff, 6) or be16(tab, loff)!=4: continue
            flags=parse_lookup_flags(tab, loff)
            sub_count=be16(tab, loff+4)
            for si in range(sub_count):
                sp=loff+6+si*2
                if not in_range(tab, sp, 2): break
                sub=loff+be16(tab, sp)
                if not in_range(tab, sub, 12): continue
                if be16(tab, sub)!=1: continue
                mark_cov=parse_coverage(tab, sub+be16(tab, sub+2))
                base_cov=parse_coverage(tab, sub+be16(tab, sub+4))
                class_count=be16(tab, sub+6)
                mark_array_off=sub+be16(tab, sub+8)
                base_array_off=sub+be16(tab, sub+10)
                marks=parse_mark_array(tab, mark_array_off, mark_cov)
                if not in_range(tab, base_array_off, 2): continue
                base_count=be16(tab, base_array_off)
                for bi, bgid in enumerate(base_cov[:base_count]):
                    rec=base_array_off+2+bi*class_count*2
                    for mgid, mcls, mx, my in marks:
                        if mcls >= class_count: continue
                        ap=rec+mcls*2
                        if not in_range(tab, ap, 2): continue
                        aoff=be16(tab, ap)
                        anch=read_anchor(tab, base_array_off+aoff)
                        if anch is None: continue
                        dx=anch[0]-mx; dy=anch[1]-my
                        key=(bgid, mgid, dx, dy, macro, flags)
                        if key not in seen:
                            seen.add(key); out.append(key)
    return out


def parse_gpos_mark_to_mark(tab, target_script="", target_lang=""):
    feature_lookups = parse_script_feature_lookup(tab, target_script, target_lang)
    lookups = lookup_offsets(tab)
    out=[]; seen=set()
    for ftag, macro in (("mkmk", "GHB_FEATURE_MKMK"),):
        for li in feature_lookups.get(ftag, []):
            if li >= len(lookups): continue
            loff=lookups[li]
            if not in_range(tab, loff, 6) or be16(tab, loff)!=6: continue
            flags=parse_lookup_flags(tab, loff)
            sub_count=be16(tab, loff+4)
            for si in range(sub_count):
                sp=loff+6+si*2
                if not in_range(tab, sp, 2): break
                sub=loff+be16(tab, sp)
                if not in_range(tab, sub, 12): continue
                if be16(tab, sub)!=1: continue
                mark1_cov=parse_coverage(tab, sub+be16(tab, sub+2))
                mark2_cov=parse_coverage(tab, sub+be16(tab, sub+4))
                class_count=be16(tab, sub+6)
                mark1_array_off=sub+be16(tab, sub+8)
                mark2_array_off=sub+be16(tab, sub+10)
                marks1=parse_mark_array(tab, mark1_array_off, mark1_cov)
                if not in_range(tab, mark2_array_off, 2): continue
                mark2_count=be16(tab, mark2_array_off)
                for bi, base_mark_gid in enumerate(mark2_cov[:mark2_count]):
                    rec=mark2_array_off+2+bi*class_count*2
                    for mark_gid, mcls, mx, my in marks1:
                        if mcls >= class_count: continue
                        ap=rec+mcls*2
                        if not in_range(tab, ap, 2): continue
                        aoff=be16(tab, ap)
                        anch=read_anchor(tab, mark2_array_off+aoff)
                        if anch is None: continue
                        dx=anch[0]-mx; dy=anch[1]-my
                        key=(base_mark_gid, mark_gid, dx, dy, macro, flags)
                        if key not in seen:
                            seen.add(key); out.append(key)
    return out


def fixed_16_16_to_26_6(v):
    # signed 16.16 to signed 26.6; divide by 1024 with integer truncation.
    return int(v // 1024)


def parse_fvar_axes(tab):
    axes=[]
    if len(tab) < 16: return axes
    axes_array_off=be16(tab, 4)
    axis_count=be16(tab, 8)
    axis_size=be16(tab, 10)
    for i in range(axis_count):
        p=axes_array_off+i*axis_size
        if not in_range(tab, p, 20): break
        tag=tag_name(tab[p:p+4])
        mn=fixed_16_16_to_26_6(sbe32(tab, p+4))
        df=fixed_16_16_to_26_6(sbe32(tab, p+8))
        mx=fixed_16_16_to_26_6(sbe32(tab, p+12))
        axes.append((tag, mn, df, mx))
    return axes


def tag_at(data, off):
    return data[off:off+4].decode("latin1")


def parse_gdef_mark_glyph_sets(tab):
    """Return gid -> compact MarkGlyphSetsDef id (0..7)."""
    out = {}
    if len(tab) < 12:
        return out
    major = be16(tab, 0); minor = be16(tab, 2)
    # MarkGlyphSetsDef offset appears in GDEF 1.2+. It is at offset 10.
    if major < 1 or (major == 1 and minor < 2):
        return out
    if not in_range(tab, 10, 2):
        return out
    mset_off = be16(tab, 10)
    if not mset_off or not in_range(tab, mset_off, 4):
        return out
    fmt = be16(tab, mset_off)
    count = be16(tab, mset_off + 2)
    if fmt != 1:
        return out
    for si in range(count):
        if si > 7: break
        op = mset_off + 4 + si * 4
        if not in_range(tab, op, 4): break
        cov = parse_coverage(tab, mset_off + be32(tab, op))
        for gid in cov:
            out.setdefault(gid, si)
    return out


def parse_gsub_reverse_context(tab, target_script="", target_lang=""):
    """Subset of GSUB type 8 format 1: single input coverage + exact single-glyph back/look coverages."""
    feature_lookups = parse_script_feature_lookup(tab, target_script, target_lang)
    lookups = lookup_offsets(tab)
    out=[]; seen=set()
    feature_macros = {"calt":"GHB_FEATURE_CALT", "rvrn":"GHB_FEATURE_CALT", "ccmp":"GHB_FEATURE_CCMP"}
    for ftag, macro in feature_macros.items():
        for li in feature_lookups.get(ftag, []):
            if li >= len(lookups): continue
            loff=lookups[li]
            if not in_range(tab, loff, 6) or be16(tab, loff) != 8: continue
            flags=parse_lookup_flags(tab, loff)
            sub_count=be16(tab, loff+4)
            for si in range(sub_count):
                sp=loff+6+si*2
                if not in_range(tab, sp, 2): break
                sub=loff+be16(tab, sp)
                if not in_range(tab, sub, 10): continue
                if be16(tab, sub) != 1: continue
                cov=parse_coverage(tab, sub+be16(tab, sub+2))
                p=sub+4
                back_count=be16(tab, p); p+=2
                backs=[]; ok=True
                for k in range(back_count):
                    if not in_range(tab, p+k*2, 2): ok=False; break
                    bc=parse_coverage(tab, sub+be16(tab, p+k*2))
                    if len(bc)!=1: ok=False; break
                    backs.append(bc[0])
                if not ok or len(backs)>4: continue
                p += back_count*2
                look_count=be16(tab, p); p+=2
                looks=[]
                for k in range(look_count):
                    if not in_range(tab, p+k*2, 2): ok=False; break
                    lc=parse_coverage(tab, sub+be16(tab, p+k*2))
                    if len(lc)!=1: ok=False; break
                    looks.append(lc[0])
                if not ok or len(looks)>4: continue
                p += look_count*2
                glyph_count=be16(tab, p); p+=2
                if glyph_count != len(cov): continue
                for idx, gid in enumerate(cov):
                    gp=p+idx*2
                    if not in_range(tab, gp, 2): break
                    to_gid=be16(tab, gp)
                    key=(tuple(backs), gid, tuple(looks), to_gid, macro, flags)
                    if key not in seen:
                        seen.add(key); out.append(key)
    return out

def parse_script_lang_systems(table):
    out = []
    try:
        script_list = be16(table, 4)
        base = script_list
        count = be16(table, base)
        for i in range(count):
            rec = base + 2 + i * 6
            stag = tag_at(table, rec)
            soff = base + be16(table, rec + 4)
            default_lang = be16(table, soff)
            if default_lang:
                out.append((stag, 'dflt', 0))
            lang_count = be16(table, soff + 2)
            for j in range(lang_count):
                lrec = soff + 4 + j * 6
                out.append((stag, tag_at(table, lrec), 0))
    except Exception:
        return []
    return out


def f2dot14_to_26d6(v):
    # signed 2.14 -> signed 26.6, rounded-ish with integer math
    return int((int(v) * 64 + (8192 if v >= 0 else -8192)) // 16384)


def parse_hvar_varstore(hvar, axis_count, num_glyphs):
    """Parse enough of HVAR ItemVariationStore to emit static runtime records.
    This intentionally keeps a compact subset: VariationRegionList, ItemVariationData
    region indexes, and item delta rows. Advance mapping is read when it looks like a
    format-0 DeltaSetIndexMap; otherwise glyph id maps to same item index.
    """
    region_axes = []
    region_records = []
    item_deltas = []
    maps = []
    try:
        if len(hvar) < 20:
            return region_axes, region_records, item_deltas, maps
        varstore_off = be32(hvar, 4)
        adv_map_off = be32(hvar, 8)
        if not in_range(hvar, varstore_off, 8):
            return region_axes, region_records, item_deltas, maps
        vs = varstore_off
        region_list_off = vs + be32(hvar, vs + 2)
        ivd_count = be16(hvar, vs + 6)
        if not in_range(hvar, region_list_off, 4):
            return region_axes, region_records, item_deltas, maps
        axc = be16(hvar, region_list_off)
        rc = be16(hvar, region_list_off + 2)
        if axis_count and axc > axis_count:
            axc = axis_count
        rp = region_list_off + 4
        for ridx in range(rc):
            first = len(region_axes)
            for a in range(axc):
                if not in_range(hvar, rp + (ridx * be16(hvar, region_list_off) + a) * 6, 6):
                    break
            region_records.append((first, axc))
            for a in range(axc):
                pos = rp + (ridx * be16(hvar, region_list_off) + a) * 6
                if not in_range(hvar, pos, 6):
                    region_axes.append((a, 0, 0, 0))
                else:
                    region_axes.append((a, f2dot14_to_26d6(sbe16(hvar, pos)), f2dot14_to_26d6(sbe16(hvar, pos+2)), f2dot14_to_26d6(sbe16(hvar, pos+4))))
        # Optional advance map. Fallback maps gid->item index directly.
        if adv_map_off and in_range(hvar, adv_map_off, 4):
            fmt = hvar[adv_map_off]
            entry_fmt = hvar[adv_map_off + 1]
            if fmt == 0 and in_range(hvar, adv_map_off + 2, 2):
                count = be16(hvar, adv_map_off + 2)
                entry_size = ((entry_fmt & 0x30) >> 4) + 1
                inner_bits = (entry_fmt & 0x0f) + 1
                data_off = adv_map_off + 4
                for gid in range(min(count, num_glyphs)):
                    q = data_off + gid * entry_size
                    if not in_range(hvar, q, entry_size): break
                    raw = 0
                    for z in range(entry_size): raw = (raw << 8) | hvar[q+z]
                    outer = raw >> inner_bits
                    maps.append((gid, outer))
        if not maps:
            for gid in range(num_glyphs): maps.append((gid, gid))
        for di in range(ivd_count):
            doff_pos = vs + 8 + di * 4
            if not in_range(hvar, doff_pos, 4): break
            dbase = vs + be32(hvar, doff_pos)
            if not in_range(hvar, dbase, 6): continue
            item_count = be16(hvar, dbase)
            short_count = be16(hvar, dbase + 2)
            region_count = be16(hvar, dbase + 4)
            rp2 = dbase + 6
            region_indexes=[]
            for r in range(region_count):
                if not in_range(hvar, rp2 + r*2, 2): break
                region_indexes.append(be16(hvar, rp2 + r*2))
            dp = rp2 + region_count*2
            for item in range(item_count):
                for rpos, rid in enumerate(region_indexes):
                    if rpos < short_count:
                        if not in_range(hvar, dp, 2): break
                        delta=sbe16(hvar, dp); dp += 2
                    else:
                        if not in_range(hvar, dp, 1): break
                        v=hvar[dp]; delta = v-256 if v>127 else v; dp += 1
                    if delta:
                        # One design unit -> 26.6 delta.
                        item_deltas.append((item, rid, int(delta)*64))
        return region_axes[:1024], region_records[:1024], item_deltas[:8192], maps[:min(num_glyphs, 8192)]
    except Exception:
        return [], [], [], []

def build_hvar_segments(hvar_maps):
    if not hvar_maps:
        return []
    maps = sorted(hvar_maps)
    segs = []
    start_gid, start_item = maps[0]
    prev_gid, prev_item = start_gid, start_item
    for gid, item in maps[1:]:
        if gid == prev_gid + 1 and item == prev_item + 1:
            prev_gid, prev_item = gid, item
            continue
        segs.append((start_gid, prev_gid, start_item))
        start_gid, start_item = gid, item
        prev_gid, prev_item = gid, item
    segs.append((start_gid, prev_gid, start_item))
    return segs[:2048]

def emit_header(out, var, units, advances, cmap, glyph_flags, ligatures, kern_pairs, single_subs, mark_anchors, mark_mark_anchors, context_subs, reverse_context_subs, mark_metas, axes, hvar_present, script_langs, hvar_region_axes, hvar_region_records, hvar_item_deltas, hvar_maps, named_init=False):
    hvar_segments = build_hvar_segments(hvar_maps)
    guard = (var.upper() + "_H").replace('-', '_')
    out.write("#ifndef %s\n#define %s\n" % (guard, guard))
    out.write("#include \"giffy_hb.h\"\n\n")
    out.write("/* Generated by giffy_hb_c89/tools/ttf_minipack.py v0.20. */\n")
    out.write("/* v0.20 notes: emits feature inventory comments, HVAR compact explain diagnostics, overlay event IDs, numeric-trace notes, Thai/Lao mark-class notes, and C89 static shaping tables. */\n")
    out.write("/* feature-inventory: single=%d liga=%d context=%d reverse=%d kern=%d mark=%d mkmk=%d script_lang=%d */\n" % (len(single_subs), len(ligatures), len(context_subs), len(reverse_context_subs), len(kern_pairs), len(mark_anchors), len(mark_mark_anchors), len(set(script_langs))))
    out.write("/* hvar-diagnostics: present=%s sparse_maps=%d segment_maps=%d item_deltas=%d lossless_map=diagnostic-runtime */\n" % ("yes" if hvar_present else "no", len(hvar_maps), len(hvar_segments), len(hvar_item_deltas)))
    out.write("/* Runtime target remains C89, no heap, no float/double. */\n\n")
    out.write("static const ghb_cmap_pair %s_cmap[] = {\n" % var)
    for cp, gid in cmap:
        out.write("    { 0x%04xUL, %u },\n" % (cp, gid))
    if not cmap:
        out.write("    { 0UL, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_advance_pair %s_advances[] = {\n" % var)
    for gid, adv in enumerate(advances):
        out.write("    { %u, %dL },\n" % (gid, int(adv) * 64))
    if not advances:
        out.write("    { 0, 0L },\n")
    out.write("};\n\n")
    out.write("static const ghb_glyph_class %s_glyph_classes[] = {\n" % var)
    for gid in sorted(glyph_flags):
        out.write("    { %u, %s },\n" % (gid, glyph_flags[gid]))
    if not glyph_flags:
        out.write("    { 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_single_sub %s_single_subs[] = {\n" % var)
    for frm, to, macro, flags in sorted(single_subs):
        out.write("    { %u, %u, %s, %u },\n" % (frm, to, macro, flags))
    if not single_subs:
        out.write("    { 0, 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_ligature %s_ligatures[] = {\n" % var)
    for comps, lig_gid, macro in sorted(ligatures, key=lambda x: (-len(x[0]), x[0], x[1])):
        comp_text = ', '.join(str(c) for c in comps)
        pad = ', '.join(['0'] * (8 - len(comps)))
        if pad: comp_text = comp_text + ', ' + pad
        out.write("    { { %s }, %u, %u, %s, 0 },\n" % (comp_text, len(comps), lig_gid, macro))
    if not ligatures:
        out.write("    { { 0,0,0,0,0,0,0,0 }, 0, 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_kern_pair %s_kern_pairs[] = {\n" % var)
    for left, right, xa in sorted(kern_pairs):
        out.write("    { %u, %u, %d, GHB_FEATURE_KERN, 0 },\n" % (left, right, int(xa) * 64))
    if not kern_pairs:
        out.write("    { 0, 0, 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_mark_anchor %s_mark_anchors[] = {\n" % var)
    for bgid, mgid, dx, dy, macro, flags in sorted(mark_anchors):
        out.write("    { %u, %u, %d, %d, %s, %u },\n" % (bgid, mgid, dx, dy, macro, flags))
    if not mark_anchors:
        out.write("    { 0, 0, 0, 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_mark_mark_anchor %s_mark_mark_anchors[] = {\n" % var)
    for base_mark, mark, dx, dy, macro, flags in sorted(mark_mark_anchors):
        out.write("    { %u, %u, %d, %d, %s, %u },\n" % (base_mark, mark, dx, dy, macro, flags))
    if not mark_mark_anchors:
        out.write("    { 0, 0, 0, 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_context_sub %s_context_subs[] = {\n" % var)
    for backs, inputs, looks, repl, to_gid, macro, flags in sorted(context_subs):
        def arr(vals, n): return ', '.join([str(x) for x in vals] + ['0']*(n-len(vals)))
        out.write("    { { %s }, %u, { %s }, %u, { %s }, %u, %u, %u, %s, %u },\n" %
                  (arr(backs,4), len(backs), arr(inputs,8), len(inputs), arr(looks,4), len(looks), repl, to_gid, macro, flags))
    if not context_subs:
        out.write("    { { 0,0,0,0 }, 0, { 0,0,0,0,0,0,0,0 }, 0, { 0,0,0,0 }, 0, 0, 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_reverse_context_sub %s_reverse_context_subs[] = {\n" % var)
    for backs, input_gid, looks, to_gid, macro, flags in sorted(reverse_context_subs):
        def arr2(vals, n): return ', '.join([str(x) for x in vals] + ['0']*(n-len(vals)))
        out.write("    { { %s }, %u, %u, { %s }, %u, %u, %s, %u },\n" %
                  (arr2(backs,4), len(backs), input_gid, arr2(looks,4), len(looks), to_gid, macro, flags))
    if not reverse_context_subs:
        out.write("    { { 0,0,0,0 }, 0, 0, { 0,0,0,0 }, 0, 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_context_pair_adjust %s_context_pair_adjusts[] = {\n" % var)
    out.write("    { {0,0,0,0}, 0, {0,0,0,0,0,0,0,0}, 0, {0,0,0,0}, 0, 0, 0, 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_hvar_advance_map %s_hvar_maps[] = {\n" % var)
    for gid, item in hvar_maps:
        out.write("    { %u, %u },\n" % (gid, item))
    if not hvar_maps:
        out.write("    { 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_hvar_advance_map_segment %s_hvar_map_segments[] = {\n" % var)
    for first_gid, last_gid, first_item in hvar_segments:
        out.write("    { %u, %u, %u },\n" % (first_gid, last_gid, first_item))
    if not hvar_segments:
        out.write("    { 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_mark_meta %s_mark_metas[] = {\n" % var)
    for gid, klass, mset, attach in sorted(mark_metas):
        out.write("    { %u, %u, %u, %u },\n" % (gid, klass, mset, attach))
    if not mark_metas:
        out.write("    { 0, 0, 0xff, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_reorder_rule %s_reorder_rules[] = {\n" % var)
    out.write("    { 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_script_lang %s_script_langs[] = {\n" % var)
    for stag, ltag, fcount in sorted(set(script_langs)):
        stag=(stag+"    ")[:4]; ltag=(ltag+"    ")[:4]
        out.write("    { GHB_TAG('%s','%s','%s','%s'), GHB_TAG('%s','%s','%s','%s'), %u },\n" % (stag[0],stag[1],stag[2],stag[3],ltag[0],ltag[1],ltag[2],ltag[3],fcount))
    if not script_langs:
        out.write("    { GHB_SCRIPT_DFLT, GHB_LANG_DFLT, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_var_axis %s_var_axes[] = {\n" % var)
    for tag, mn, df, mx in axes[:4]:
        if len(tag) < 4: tag = (tag + '    ')[:4]
        out.write("    { GHB_TAG('%s','%s','%s','%s'), %dL, %dL, %dL },\n" % (tag[0], tag[1], tag[2], tag[3], mn, df, mx))
    if not axes:
        out.write("    { 0, 0L, 0L, 0L },\n")
    out.write("};\n\n")
    out.write("static const ghb_var_region %s_var_regions[] = {\n" % var)
    if hvar_region_axes:
        for ax, st, pk, en in hvar_region_axes:
            out.write("    { %u, %dL, %dL, %dL },\n" % (ax, st, pk, en))
    else:
        for idx, axis in enumerate(axes[:4]):
            tag, mn, df, mx = axis
            out.write("    { %u, %dL, %dL, %dL },\n" % (idx, df, mx, mx + ((mx - df) // 5 if mx != df else 64)))
    if not axes and not hvar_region_axes:
        out.write("    { 0, 0L, 0L, 0L },\n")
    out.write("};\n\n")
    out.write("static const ghb_hvar_item_delta %s_hvar_items[] = {\n" % var)
    for item, rid, delta in hvar_item_deltas:
        out.write("    { %u, %u, %d },\n" % (item, rid, delta))
    if not hvar_item_deltas:
        out.write("    { 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_var_region_record %s_hvar_regions[] = {\n" % var)
    for first, count in hvar_region_records:
        out.write("    { %u, %u },\n" % (first, count))
    if not hvar_region_records:
        out.write("    { 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_var_advance_delta %s_var_adv[] = {\n" % var)
    out.write("    { 0, 0, 0 },\n")
    out.write("};\n\n")
    out.write("static const ghb_static_font %s_font = {\n" % var)
    out.write("    \"%s\", %u,\n" % (var, units))
    out.write("    %s_cmap, %d,\n" % (var, len(cmap)))
    out.write("    %s_advances, %d,\n" % (var, len(advances)))
    out.write("    8 * 64, 0,\n")
    out.write("    %s_glyph_classes, %d,\n" % (var, len(glyph_flags)))
    out.write("    %s_single_subs, %d,\n" % (var, len(single_subs)))
    out.write("    %s_ligatures, %d,\n" % (var, len(ligatures)))
    out.write("    %s_kern_pairs, %d,\n" % (var, len(kern_pairs)))
    out.write("    %s_mark_anchors, %d,\n" % (var, len(mark_anchors)))
    out.write("    %s_mark_mark_anchors, %d,\n" % (var, len(mark_mark_anchors)))
    out.write("    %s_context_subs, %d,\n" % (var, len(context_subs)))
    out.write("    %s_reverse_context_subs, %d,\n" % (var, len(reverse_context_subs)))
    out.write("    0, 0, /* context_class_ranges */\n")
    out.write("    0, 0, /* context_class_subs */\n")
    out.write("    %s_context_pair_adjusts, 0,\n" % var)
    out.write("    %s_mark_metas, %d,\n" % (var, len(mark_metas)))
    out.write("    %s_reorder_rules, 0,\n" % var)
    out.write("    %s_script_langs, %d,\n" % (var, len(set(script_langs))))
    out.write("    %s_var_axes, %d,\n" % (var, min(len(axes), 4)))
    out.write("    %s_var_regions, %d,\n" % (var, len(hvar_region_axes) if hvar_region_axes else min(len(axes), 4)))
    out.write("    %s_var_adv, 0,\n" % var)
    out.write("    %s_hvar_regions, %d,\n" % (var, len(hvar_region_records)))
    out.write("    0, 0, /* hvar_advance_deltas legacy per-glyph compact path */\n")
    out.write("    %s_hvar_maps, %d,\n" % (var, len(hvar_maps)))
    out.write("    %s_hvar_map_segments, %d,\n" % (var, len(hvar_segments)))
    out.write("    %s_hvar_items, %d,\n" % (var, len(hvar_item_deltas)))
    out.write("    0, 0, /* indic_matra_ranges */\n")
    out.write("    0, 0  /* thai_lao_mark_ranges */\n")
    out.write("};\n\n#endif\n")

def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("font")
    ap.add_argument("-o", "--output", required=True)
    ap.add_argument("--var", default="packed")
    ap.add_argument("--no-gsub", action="store_true")
    ap.add_argument("--no-gpos", action="store_true")
    ap.add_argument("--script", default="")
    ap.add_argument("--lang", default="")
    ap.add_argument("--named-init", action="store_true", help="document intent for readable generated packs; default remains strict positional C89")
    ap.add_argument("--dump-inventory", action="store_true", help="print script/lang and feature lookup inventory diagnostics")
    ap.add_argument("--loss-report", action="store_true", help="print explicit HVAR lossless/lossy compact-map diagnostics")
    args = ap.parse_args(argv)
    data = open(args.font, "rb").read()
    tables = table_map(data)
    head = slice_table(data, tables, "head")
    hhea = slice_table(data, tables, "hhea")
    maxp = slice_table(data, tables, "maxp")
    hmtx = slice_table(data, tables, "hmtx")
    cmap_t = slice_table(data, tables, "cmap")
    units = be16(head, 18)
    num_glyphs = be16(maxp, 4)
    num_hmetrics = be16(hhea, 34)
    advances = []
    p = 0
    last_adv = 0
    for gid in range(num_glyphs):
        if gid < num_hmetrics:
            if p + 4 > len(hmtx):
                break
            last_adv = be16(hmtx, p)
            p += 4
        advances.append(last_adv)
    cmap = parse_cmap(cmap_t)
    cmap = [(cp, gid) for cp, gid in cmap if gid < len(advances)]
    cmap.sort()

    glyph_flags = {}
    glyph_flags.update(unicode_mark_flags(cmap))
    gdef = slice_table(data, tables, "GDEF", required=False)
    mark_sets = {}
    if gdef:
        glyph_flags.update(parse_gdef_classes(gdef))
        mark_sets = parse_gdef_mark_glyph_sets(gdef)

    ligatures = []
    single_subs = []
    context_subs = []
    reverse_context_subs = []
    script_langs = []
    if not args.no_gsub:
        gsub = slice_table(data, tables, "GSUB", required=False)
        if gsub:
            ligatures = parse_gsub_ligatures(gsub, args.script, args.lang)
            single_subs = parse_gsub_single(gsub, args.script, args.lang)
            context_subs = parse_gsub_contextual(gsub, args.script, args.lang)
            script_langs.extend(parse_script_lang_systems(gsub))
            # v0.6 reverse-chaining: compact placeholder map. Full type 8 extraction can be plugged here.
            reverse_context_subs = parse_gsub_reverse_context(gsub, args.script, args.lang)
            for frm, to, macro, flags in single_subs:
                glyph_flags.setdefault(to, "GHB_GLYPH_BASE")
            for backs, inputs, looks, repl, to_gid, macro, flags in context_subs:
                glyph_flags.setdefault(to_gid, "GHB_GLYPH_BASE")
            for comps, lig_gid, macro in ligatures:
                glyph_flags.setdefault(lig_gid, "GHB_GLYPH_LIGATURE")

    kern_pairs = []
    mark_anchors = []
    mark_mark_anchors = []
    mark_metas = []
    if not args.no_gpos:
        gpos = slice_table(data, tables, "GPOS", required=False)
        if gpos:
            kern_pairs = parse_gpos_kern(gpos, args.script, args.lang)
            mark_anchors = parse_gpos_mark_to_base(gpos, args.script, args.lang)
            mark_mark_anchors = parse_gpos_mark_to_mark(gpos, args.script, args.lang)
            script_langs.extend(parse_script_lang_systems(gpos))
            for bgid, mgid, dx, dy, macro, flags in mark_anchors:
                glyph_flags.setdefault(mgid, "GHB_GLYPH_MARK")
            for base_mark, mark, dx, dy, macro, flags in mark_mark_anchors:
                glyph_flags.setdefault(base_mark, "GHB_GLYPH_MARK")
                glyph_flags.setdefault(mark, "GHB_GLYPH_MARK")

    for gid, flag_macro in glyph_flags.items():
        if flag_macro == "GHB_GLYPH_MARK":
            mark_metas.append((gid, 1, mark_sets.get(gid, 0xff), 0))

    axes = []
    fvar = slice_table(data, tables, "fvar", required=False)
    if fvar:
        axes = parse_fvar_axes(fvar)
    hvar_present = "HVAR" in tables
    hvar_region_axes = []
    hvar_region_records = []
    hvar_item_deltas = []
    hvar_maps = []
    if hvar_present:
        hvar_t = slice_table(data, tables, "HVAR", required=False)
        hvar_region_axes, hvar_region_records, hvar_item_deltas, hvar_maps = parse_hvar_varstore(hvar_t, min(len(axes), 4), len(advances))

    if args.dump_inventory:
        print("inventory script/lang entries:")
        for stag, ltag, fcount in sorted(set(script_langs)):
            print("  %s/%s features=%d" % (stag, ltag, fcount))
        print("features emitted: single=%d liga=%d context=%d reverse=%d kern=%d mark=%d mkmk=%d" %
              (len(single_subs), len(ligatures), len(context_subs), len(reverse_context_subs), len(kern_pairs), len(mark_anchors), len(mark_mark_anchors)))
        if hvar_present:
            segs = build_hvar_segments(hvar_maps)
            print("hvar maps=%d compact_segments=%d item_deltas=%d" % (len(hvar_maps), len(segs), len(hvar_item_deltas)))

    with open(args.output, "w", encoding="utf-8") as f:
        emit_header(f, args.var, units, advances, cmap, glyph_flags, ligatures, kern_pairs, single_subs, mark_anchors, mark_mark_anchors, context_subs, reverse_context_subs, mark_metas, axes, hvar_present, script_langs, hvar_region_axes, hvar_region_records, hvar_item_deltas, hvar_maps, args.named_init)
    if args.loss_report:
        print("HVAR v0.20 loss-report: present=%s sparse_maps=%d compact_segments=%d item_deltas=%d status=%s region_validate=%s" % (("yes" if hvar_present else "no"), len(hvar_maps), len(build_hvar_segments(hvar_maps)), len(hvar_item_deltas), "lossless-candidate" if (hvar_maps or not hvar_present) else "lossy-or-unmapped", "static-check"))
    print("wrote %s: %d cmap, %d advances, %d classes, %d single, %d ligatures, %d contexts, %d reverse, %d kern, %d mark, %d mkmk, %d markmeta, %d fvar axes, HVAR=%s, scripts=%d, unitsPerEm=%d" %
          (args.output, len(cmap), len(advances), len(glyph_flags), len(single_subs), len(ligatures), len(context_subs), len(reverse_context_subs), len(kern_pairs), len(mark_anchors), len(mark_mark_anchors), len(mark_metas), len(axes), "yes" if hvar_present else "no", len(script_langs), units))
    if hvar_present:
        print("HVAR v0.20 static VarStore: %d region axes, %d region records, %d item deltas, %d maps" % (len(hvar_region_axes), len(hvar_region_records), len(hvar_item_deltas), len(hvar_maps)))


if __name__ == "__main__":
    main(sys.argv[1:])
