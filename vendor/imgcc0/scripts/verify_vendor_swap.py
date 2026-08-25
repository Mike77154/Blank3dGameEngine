from pathlib import Path
import hashlib, sys
ROOT=Path(__file__).resolve().parents[1]
ok=True

def load_manifest(path):
    expected={}
    for line in path.read_text().splitlines():
        if not line.strip():
            continue
        h,rel=line.split('  ',1)
        expected[rel]=h
    return expected

def scan_tree(base):
    actual={}
    for p in sorted(x for x in base.rglob('*') if x.is_file()):
        actual[p.relative_to(base).as_posix()]=hashlib.sha256(p.read_bytes()).hexdigest()
    return actual

def compare(name, expected, actual, label):
    global ok
    missing=sorted(set(expected)-set(actual))
    extra=sorted(set(actual)-set(expected))
    changed=sorted(k for k in set(expected)&set(actual) if expected[k]!=actual[k])
    if missing or extra or changed:
        ok=False
        print('FAIL vendor',name,label,'missing',len(missing),'extra',len(extra),'changed',len(changed))
        for kind,items in (('missing',missing),('extra',extra),('changed',changed)):
            for item in items[:20]:
                print(' ',kind,item)
    else:
        print('PASS vendor %s: %d files match %s'%(name,len(expected),label))

for name in ('PNG','DDS','BMP','PCX','PSD'):
    expected=load_manifest(ROOT/'vendor_manifests'/(name+'.sha256'))
    compare(name,expected,scan_tree(ROOT/'codecs'/name),'supplied package')

# ZRAGF is intentionally repaired in this revision.  Preserve the original
# supplied-package manifest and require that the only upstream delta is the
# audited zragf_stream.c protocol fix; then verify the full repaired tree.
z_orig=load_manifest(ROOT/'vendor_manifests'/'ZRAGF.sha256')
z_fixed=load_manifest(ROOT/'vendor_manifests'/'ZRAGF_PROTOCOL89_FIXED.sha256')
manifest_delta=sorted(k for k in set(z_orig)&set(z_fixed) if z_orig[k]!=z_fixed[k])
manifest_missing=sorted(set(z_orig)-set(z_fixed))
manifest_extra=sorted(set(z_fixed)-set(z_orig))
if manifest_delta != ['zragf_stream.c'] or manifest_missing or manifest_extra:
    ok=False
    print('FAIL ZRAGF repaired manifest does not describe exactly one intentional source delta')
    print(' delta',manifest_delta,'missing',manifest_missing,'extra',manifest_extra)
else:
    print('PASS ZRAGF patch scope: only zragf_stream.c differs from supplied package')
compare('ZRAGF',z_fixed,scan_tree(ROOT/'codecs'/'ZRAGF'),'Protocol89 repaired manifest')

if (ROOT/'codecs'/'PSD').exists():
    print('PASS PSD replacement vendor present')
else:
    ok=False
    print('FAIL PSD replacement vendor missing')
sys.exit(0 if ok else 1)
