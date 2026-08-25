#!/usr/bin/env python3
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
MOD = HERE.parent
ROOT = MOD.parent.parent
SRC = MOD / 'legacy_reference'
OUT = ROOT / 'config' / 'reticles'
PRESETS = OUT / 'presets'

shape_src = (SRC / 'gscopepresets89.c').read_text(encoding='utf-8') + '\n' + (SRC / 'gscopepresets89_expanded_shapes.inc').read_text(encoding='utf-8')
registry_src = (SRC / 'gscopepresets89.c').read_text(encoding='utf-8') + '\n' + (SRC / 'gscopepresets89_expanded_registry.inc').read_text(encoding='utf-8')

FIELDS = ['kind','part_id','flags','layer','thickness_px','outline_px','x0','y0','x1','y1','x2','y2','x3','y3','a','b','c','d','i0','i1','i2','i3','points','point_count']
KIND_PREFIX='GSV89_SHAPE_'
PART_PREFIX='GSV89_PART_'

I_ENUM = {
    'GSV89_DIR_UP':0,'GSV89_DIR_DOWN':1,'GSV89_DIR_LEFT':2,'GSV89_DIR_RIGHT':3,
    'GSV89_AXIS_HORIZONTAL':0,'GSV89_AXIS_VERTICAL':1,
}
PRESET_FAMILY = {
    'GSVP89_FAMILY_CLASSIC':1,'GSVP89_FAMILY_TACTICAL':2,'GSVP89_FAMILY_HISTORICAL':3,
    'GSVP89_FAMILY_HUNTING':4,'GSVP89_FAMILY_DIGITAL':5,'GSVP89_FAMILY_LAUNCHER':6,
}
FOCAL = {'GSVP89_FOCAL_FIXED_SCREEN':0,'GSVP89_FOCAL_FIRST':1,'GSVP89_FOCAL_SECOND':2}
THEME = {'GSVP89_THEME_BLACK':0,'GSVP89_THEME_RED':1,'GSVP89_THEME_GREEN':2,'GSVP89_THEME_AMBER':3,'GSVP89_THEME_WHITE':4,'GSVP89_THEME_DUAL':5}
PFLAG = {'GSVP89_FLAG_PURE_VECTOR':1,'GSVP89_FLAG_HISTORICAL':2,'GSVP89_FLAG_INSPIRED':4,'GSVP89_FLAG_ILLUMINATED':8,'GSVP89_FLAG_LAUNCHER':16,'GSVP89_FLAG_RANGEFINDER':32,'GSVP89_FLAG_BDC':64}
SFLAG = {'GSV89_FLAG_FILLED':1,'GSV89_FLAG_CLOSED':2,'GSV89_FLAG_VISIBLE':16,'GSV89_FLAG_OUTLINE':32,'GSV89_FLAG_MAJOR_ALT':64,'GSV89_FLAG_FLIP':128}

def split_top(s):
    out=[]; cur=[]; depth=0; in_str=False; esc=False
    for ch in s:
        if in_str:
            cur.append(ch)
            if esc: esc=False
            elif ch=='\\': esc=True
            elif ch=='"': in_str=False
            continue
        if ch=='"': in_str=True; cur.append(ch); continue
        if ch=='(': depth+=1
        elif ch==')': depth-=1
        if ch==',' and depth==0:
            out.append(''.join(cur).strip()); cur=[]
        else: cur.append(ch)
    out.append(''.join(cur).strip())
    return out

def match_brace(s,pos):
    depth=0
    for i in range(pos,len(s)):
        if s[i]=='{': depth+=1
        elif s[i]=='}':
            depth-=1
            if depth==0: return i
    raise ValueError('unmatched brace')

def eval_or(expr, mapping):
    expr=expr.replace('(short)','').replace('(',' ').replace(')',' ')
    total=0
    for tok in expr.split('|'):
        tok=tok.strip()
        if not tok: continue
        if tok in mapping: total |= mapping[tok]
        elif re.fullmatch(r'-?\d+', tok): total |= int(tok)
        else: raise ValueError('unknown token %r in %r' % (tok,expr))
    return total

def norm_val(tok):
    m=re.fullmatch(r'GSP89_NORM\((-?\d+)\)', tok)
    if m: return int(m.group(1))
    if re.fullmatch(r'-?\d+',tok):
        v=int(tok)
        if v!=0: raise ValueError('nonzero raw fixed field %s' % tok)
        return v
    raise ValueError('bad fixed token %r' % tok)

def int_val(tok):
    if tok in I_ENUM: return I_ENUM[tok]
    if re.fullmatch(r'-?\d+',tok): return int(tok)
    raise ValueError('bad integer token %r' % tok)

# arrays
arrays={}
arr_pat=re.compile(r'static\s+const\s+gsv89_shape\s+(gsvp89_shapes_[A-Za-z0-9_]+)\[\]\s*=\s*\{',re.M)
for m in arr_pat.finditer(shape_src):
    openpos=shape_src.find('{',m.end()-1); end=match_brace(shape_src,openpos); body=shape_src[openpos+1:end]
    shapes=[]; p=0
    while True:
        a=body.find('{',p)
        if a<0: break
        b=match_brace(body,a)
        vals=split_top(body[a+1:b])
        if len(vals)!=24: raise ValueError('%s shape has %d fields' % (m.group(1),len(vals)))
        sh={}
        sh['kind']=vals[0][len(KIND_PREFIX):].lower()
        sh['part_id']=vals[1][len(PART_PREFIX):].lower()
        sh['flags']=eval_or(vals[2],SFLAG)
        for idx,key in enumerate(['layer','thickness_px','outline_px'],3): sh[key]=int(vals[idx])
        for idx,key in enumerate(['x0','y0','x1','y1','x2','y2','x3','y3','a','b','c','d'],6): sh[key]=norm_val(vals[idx])
        for idx,key in enumerate(['i0','i1','i2','i3'],18): sh[key]=int_val(vals[idx])
        if vals[22] not in ('0','NULL') or int_val(vals[23])!=0: raise ValueError('point arrays not supported/current pack unexpectedly uses them')
        shapes.append(sh); p=b+1
    arrays[m.group(1)]=shapes

# registry entries, one per line
entries=[]
for line in registry_src.splitlines():
    line=line.strip()
    if not line.startswith('{ GSVP89_'): continue
    inner=line[1:line.rfind('}')]
    vals=split_top(inner)
    if len(vals)!=11: raise ValueError('registry fields %d: %s' % (len(vals),line[:120]))
    enum_name=vals[0]
    name=vals[1].strip('"')
    family_name=vals[2].strip('"')
    family=PRESET_FAMILY[vals[3]]
    flags=eval_or(vals[4],PFLAG)
    focal=FOCAL[vals[5]]
    zoom=int(vals[6]); cal=int(vals[7]); theme=THEME[vals[8]]
    arr=vals[9]
    if arr not in arrays: raise ValueError('missing array %s' % arr)
    entries.append(dict(enum=enum_name,name=name,family_name=family_name,family=family,flags=flags,focal=focal,zoom=zoom,cal=cal,theme=theme,array=arr,shapes=arrays[arr]))

if len(entries)!=192: raise SystemExit('expected 192 entries, got %d' % len(entries))
if sum(len(e['shapes']) for e in entries)!=3953: raise SystemExit('expected 3953 shapes')

PRESETS.mkdir(parents=True,exist_ok=True)
for old in PRESETS.glob('*.ini'): old.unlink()

family_names={1:'classic',2:'tactical',3:'historical',4:'hunting',5:'digital',6:'launcher'}
focal_names={0:'fixed_screen',1:'first',2:'second'}
theme_names={0:'black',1:'red',2:'green',3:'amber',4:'white',5:'dual'}
flag_names=[(1,'pure_vector'),(2,'historical'),(4,'inspired'),(8,'illuminated'),(16,'launcher'),(32,'rangefinder'),(64,'bdc')]
shape_flag_names=[(1,'filled'),(2,'closed'),(16,'visible'),(32,'outline'),(64,'major_alt'),(128,'flip')]

def flag_string(v,pairs):
    names=[name for bit,name in pairs if v & bit]
    known=sum(bit for bit,name in pairs if v & bit)
    if v & ~known: names.append(str(v & ~known))
    return '|'.join(names) if names else '0'

catalog=['; gscopepresets89 INI catalog - runtime source of truth','; Order defines numeric preset IDs for legacy index access.','; Add another name=path line to expand the catalog without recompiling.','[catalog]','version=1','','[presets]']
manifest=[]
for idx,e in enumerate(entries):
    fname='%03d_%s.ini' % (idx,e['name'])
    catalog.append('%s=presets/%s' % (e['name'],fname))
    lines=['; Auto-migrated from the old C89 vector table.','; Editable runtime recipe: no reticle geometry is compiled into gscopepresets89.c.','[preset]',
           'id=%d' % idx,'name=%s' % e['name'],'family_name=%s' % e['family_name'],'family=%s' % family_names[e['family']],
           'flags=%s' % flag_string(e['flags'],flag_names),'focal_plane=%s' % focal_names[e['focal']],
           'recommended_zoom_x100=%d' % e['zoom'],'calibration_zoom_x100=%d' % e['cal'],'default_theme=%s' % theme_names[e['theme']],
           'coord_space=norm10000','shape_count=%d' % len(e['shapes']),'']
    for si,sh in enumerate(e['shapes']):
        lines += ['[shape%03d]' % si,'kind=%s' % sh['kind'],'part=%s' % sh['part_id'],'flags=%s' % flag_string(sh['flags'],shape_flag_names)]
        for key in ['layer','thickness_px','outline_px','x0','y0','x1','y1','x2','y2','x3','y3','a','b','c','d','i0','i1','i2','i3']:
            if sh[key]!=0: lines.append('%s=%d' % (key,sh[key]))
        lines.append('')
    (PRESETS/fname).write_text('\n'.join(lines),encoding='utf-8',newline='\n')
    manifest.append('%03d %-36s %3d shapes %s' % (idx,e['name'],len(e['shapes']),fname))

(OUT/'catalog.ini').write_text('\n'.join(catalog)+'\n',encoding='utf-8',newline='\n')
(OUT/'active.ini').write_text('[reticle]\ncatalog=catalog.ini\nuse=svd_pso1_dragunov\n',encoding='utf-8',newline='\n')
(OUT/'MANIFEST_192.txt').write_text('\n'.join(manifest)+'\nTOTAL presets=192 shapes=3953\n',encoding='utf-8',newline='\n')
print('generated',len(entries),'preset INIs and',sum(len(e['shapes']) for e in entries),'shapes')
print('largest',max((len(e['shapes']),e['name']) for e in entries))
