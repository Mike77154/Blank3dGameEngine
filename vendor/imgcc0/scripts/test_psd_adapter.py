from pathlib import Path
import subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[1]
CC='cc'
direct=ROOT/'build'/'direct_psd_dump'
facade=ROOT/'build'/'imgcc0_dump_psd'
psd_sources=[
    'codecs/PSD/src/psd89_core.c','codecs/PSD/src/psd89_util.c','codecs/PSD/src/psd89_stdio.c',
    'codecs/PSD/src/psd89_zip.c','codecs/PSD/src/psd89_read.c','codecs/PSD/src/psd89_write.c',
    'codecs/PSD/src/psd89_compose.c','codecs/PSD/src/psd89_vector_bezier.c',
    'codecs/PSD/src/psd89_mask_global_tags.c','codecs/PSD/src/psd89_zip_diag.c','codecs/PSD/src/psd89_advanced.c'
]
subprocess.run([CC,'-std=c89','-pedantic-errors','-Icodecs/PSD/include','-Icodecs/PSD/src',
                'tests/direct_psd_dump.c',*psd_sources,'-lz','-o',str(direct)],cwd=ROOT,check=True)
subprocess.run([CC,'-std=c89','-pedantic-errors','-Iinclude','tests/imgcc0_dump.c','build/libimgcc0.a','-lz','-o',str(facade)],cwd=ROOT,check=True)
cases=sorted(set((ROOT/'codecs/PSD/tests').rglob('*.psd')) | set((ROOT/'codecs/PSD/examples').rglob('*.psd')))
if not cases:
    print('FAIL: no PSD samples found')
    sys.exit(1)
with tempfile.TemporaryDirectory() as td:
    td=Path(td)
    passed=0
    for i,p in enumerate(cases):
        rel=p.relative_to(ROOT).as_posix()
        a=td/(str(i)+'.direct')
        b=td/(str(i)+'.facade')
        ra=subprocess.run([str(direct),rel,str(a)],cwd=ROOT)
        rb=subprocess.run([str(facade),rel,str(b)],cwd=ROOT)
        if ra.returncode != 0 or rb.returncode != 0:
            print('FAIL PSD decode:',rel,'direct rc',ra.returncode,'facade rc',rb.returncode)
            sys.exit(1)
        ba=a.read_bytes(); bb=b.read_bytes()
        if ba != bb:
            print('FAIL PSD adapter parity:',rel,'direct=',len(ba),'facade=',len(bb))
            sys.exit(1)
        print('PASS PSD adapter parity:',rel,len(ba),'bytes')
        passed += 1
print('PSD vendor adapter parity: PASS (%d/%d byte-identical canonical outputs)'%(passed,len(cases)))
