from pathlib import Path
import subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[1]
CC='cc'
inc=[
'-Iinclude','-Icodecs/PNG','-Icodecs/DDS/include','-Icodecs/BMP','-Icodecs/BMP/include','-Icodecs/PCX','-Icodecs/PCX/include'
]
build=ROOT/'build'
direct=build/'direct_vendor_dump'
facade=build/'imgcc0_dump'
subprocess.run([CC,'-std=c89','-pedantic-errors','-DPNG_DEC_USE_ZLIB',*inc,'tests/direct_vendor_dump.c','codecs/PNG/png_zlib.c','build/libimgcc0.a','-lz','-o',str(direct)],cwd=ROOT,check=True)
subprocess.run([CC,'-std=c89','-pedantic-errors','-Iinclude','tests/imgcc0_dump.c','build/libimgcc0.a','-lz','-o',str(facade)],cwd=ROOT,check=True)
cases=[
('png','codecs/PNG/tests/corpus/png/rgba8.png'),
('png','codecs/PNG/tests/corpus/png/gray2_adam7.png'),
('apng','codecs/PNG/tests/corpus/apng/valid_three_frame.apng'),
('dds','codecs/DDS/corpus/parity_expected/dxt5.dds'),
('dds','codecs/DDS/corpus/parity_expected/bc5.dds'),
('bmp','codecs/BMP/tests/generated_corpus/valid_rgb24.bmp'),
('bmp','codecs/BMP/tests/generated_corpus/generated_indexed2.bmp'),
('pcx','codecs/PCX/tests/fuzz_corpus/valid_rgb24_round3.pcx'),
('pcx','codecs/PCX/tests/fuzz_corpus/valid_indexed8_palette.pcx'),
]
with tempfile.TemporaryDirectory() as td:
    td=Path(td)
    passed=0
    for i,(kind,rel) in enumerate(cases):
        a=td/(str(i)+'.direct')
        b=td/(str(i)+'.facade')
        subprocess.run([str(direct),kind,rel,str(a)],cwd=ROOT,check=True)
        subprocess.run([str(facade),rel,str(b)],cwd=ROOT,check=True)
        ba=a.read_bytes(); bb=b.read_bytes()
        if ba!=bb:
            print('FAIL adapter parity:',kind,rel,'direct=',len(ba),'facade=',len(bb))
            sys.exit(1)
        print('PASS adapter parity:',kind,rel,len(ba),'bytes')
        passed+=1
print('vendor adapter parity: PASS (%d byte-identical canonical outputs)'%passed)
