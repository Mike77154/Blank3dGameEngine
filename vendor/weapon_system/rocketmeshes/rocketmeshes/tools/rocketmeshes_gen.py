import math, os, json, shutil, subprocess, zipfile
ROOT='/mnt/data/rocketmeshes'
os.makedirs(os.path.join(ROOT,'include'),exist_ok=True)
os.makedirs(os.path.join(ROOT,'src'),exist_ok=True)
os.makedirs(os.path.join(ROOT,'tools'),exist_ok=True)
os.makedirs(os.path.join(ROOT,'examples'),exist_ok=True)
os.makedirs(os.path.join(ROOT,'obj'),exist_ok=True)
os.makedirs(os.path.join(ROOT,'previews'),exist_ok=True)

MAT={
    'body':0,
    'nose':1,
    'band':2,
    'fin':3,
    'motor':4,
    'tip':5,
    'detail':6,
    'brass':7,
    'empty':8,
    'smoke':9,
}
MAT_NAMES=['body','nose','band','fin','motor','tip','detail','brass','empty','smoke']

class Mesh:
    def __init__(self,cname,nice,group='projectile'):
        self.cname=cname
        self.nice=nice
        self.group=group
        self.v=[]
        self.t=[]
    def add_v(self,x,y,z):
        self.v.append((int(round(x)),int(round(y)),int(round(z))))
        return len(self.v)-1
    def add_tri(self,a,b,c,mat):
        self.t.append((a,b,c,MAT[mat] if isinstance(mat,str) else mat))
    def add_quad(self,a,b,c,d,mat):
        self.add_tri(a,b,c,mat)
        self.add_tri(a,c,d,mat)
    def add_ring(self,x,r,sides=8,phase=0.0,y0=0,z0=0,squash_z=1.0):
        ids=[]
        for i in range(sides):
            ang=2*math.pi*i/sides + phase
            y=y0+math.cos(ang)*r
            z=z0+math.sin(ang)*r*squash_z
            ids.append(self.add_v(x,y,z))
        return ids
    def add_disc_center(self,x,y0=0,z0=0):
        return self.add_v(x,y0,z0)
    def connect_rings(self,a,b,mat,reverse=False):
        n=len(a)
        for i in range(n):
            a0=a[i]; a1=a[(i+1)%n]; b0=b[i]; b1=b[(i+1)%n]
            if reverse:
                self.add_tri(a0,b1,b0,mat)
                self.add_tri(a0,a1,b1,mat)
            else:
                self.add_tri(a0,b0,b1,mat)
                self.add_tri(a0,b1,a1,mat)
    def cap_ring(self,ring,center,mat,front=True):
        n=len(ring)
        for i in range(n):
            a=ring[i]; b=ring[(i+1)%n]
            if front:
                self.add_tri(center,a,b,mat)
            else:
                self.add_tri(center,b,a,mat)
    def add_lathe(self, profile, sides=8, phase=0.0):
        # profile entries: (x,r,mat_to_next)
        rings=[]
        for x,r,mat in profile:
            if r<=0:
                center=self.add_v(x,0,0)
                rings.append([center]*sides)
            else:
                rings.append(self.add_ring(x,r,sides,phase))
        for i in range(len(rings)-1):
            mat=profile[i][2]
            a=rings[i]; b=rings[i+1]
            ra=profile[i][1]; rb=profile[i+1][1]
            if ra<=0 and rb>0:
                c=a[0]
                n=len(b)
                for j in range(n):
                    self.add_tri(c,b[j],b[(j+1)%n],mat)
            elif ra>0 and rb<=0:
                c=b[0]
                n=len(a)
                for j in range(n):
                    self.add_tri(a[j],c,a[(j+1)%n],mat)
            else:
                self.connect_rings(a,b,mat)
        return rings
    def add_fins(self,x0,x1,root_r,outer_r,count=4,width_deg=10,mat='fin',z_scale=1.0):
        for i in range(count):
            ang=2*math.pi*i/count
            half=math.radians(width_deg)/2.0
            pts=[]
            for x,r,a in [(x0,root_r,ang-half),(x0,root_r,ang+half),(x1,outer_r,ang+half),(x1,outer_r,ang-half)]:
                pts.append(self.add_v(x, math.cos(a)*r, math.sin(a)*r*z_scale))
            a,b,c,d=pts
            self.add_quad(a,b,c,d,mat)
            self.add_quad(a,d,c,b,mat)
    def add_ring_fin(self,x0,x1,inner_r,outer_r,sides=8,mat='fin'):
        r0i=self.add_ring(x0,inner_r,sides)
        r0o=self.add_ring(x0,outer_r,sides)
        r1i=self.add_ring(x1,inner_r,sides)
        r1o=self.add_ring(x1,outer_r,sides)
        self.connect_rings(r0o,r1o,mat)
        self.connect_rings(r0i,r1i,mat,reverse=True)
        n=sides
        for i in range(n):
            self.add_tri(r1i[i],r1o[i],r1o[(i+1)%n],mat)
            self.add_tri(r1i[i],r1o[(i+1)%n],r1i[(i+1)%n],mat)
            self.add_tri(r0i[i],r0o[(i+1)%n],r0o[i],mat)
            self.add_tri(r0i[i],r0i[(i+1)%n],r0o[(i+1)%n],mat)
    def add_tube(self,x0,x1,outer_r,inner_r=0,sides=8,mat_outer='empty',mat_inner='motor',y0=0,z0=0,phase=0.0,cap_back=False,cap_front=False):
        r0o=self.add_ring(x0,outer_r,sides,phase,y0,z0)
        r1o=self.add_ring(x1,outer_r,sides,phase,y0,z0)
        self.connect_rings(r0o,r1o,mat_outer)
        if inner_r>0:
            r0i=self.add_ring(x0,inner_r,sides,phase,y0,z0)
            r1i=self.add_ring(x1,inner_r,sides,phase,y0,z0)
            self.connect_rings(r1i,r0i,mat_inner)
            n=sides
            for i in range(n):
                # rear lip annulus
                self.add_tri(r0i[i],r0o[i],r0o[(i+1)%n],mat_outer)
                self.add_tri(r0i[i],r0o[(i+1)%n],r0i[(i+1)%n],mat_outer)
                # front lip annulus
                self.add_tri(r1i[i],r1o[(i+1)%n],r1o[i],mat_outer)
                self.add_tri(r1i[i],r1i[(i+1)%n],r1o[(i+1)%n],mat_outer)
            return (r0o,r1o,r0i,r1i)
        if cap_back:
            c=self.add_v(x0,y0,z0); self.cap_ring(r0o,c,mat_outer,front=False)
        if cap_front:
            c=self.add_v(x1,y0,z0); self.cap_ring(r1o,c,mat_outer,front=True)
        return (r0o,r1o)
    def add_box(self,x0,x1,y0,y1,z0,z1,mat='detail'):
        # corners
        v000=self.add_v(x0,y0,z0); v001=self.add_v(x0,y0,z1); v010=self.add_v(x0,y1,z0); v011=self.add_v(x0,y1,z1)
        v100=self.add_v(x1,y0,z0); v101=self.add_v(x1,y0,z1); v110=self.add_v(x1,y1,z0); v111=self.add_v(x1,y1,z1)
        self.add_quad(v000,v100,v110,v010,mat) # bottom-ish y0? actually side
        self.add_quad(v001,v011,v111,v101,mat)
        self.add_quad(v000,v001,v101,v100,mat)
        self.add_quad(v010,v110,v111,v011,mat)
        self.add_quad(v000,v010,v011,v001,mat)
        self.add_quad(v100,v101,v111,v110,mat)
    def add_annulus_disc(self,x,inner_r,outer_r,sides=8,mat='smoke',phase=0.0):
        ri=self.add_ring(x,inner_r,sides,phase)
        ro=self.add_ring(x,outer_r,sides,phase)
        n=sides
        for i in range(n):
            self.add_tri(ri[i],ro[i],ro[(i+1)%n],mat)
            self.add_tri(ri[i],ro[(i+1)%n],ri[(i+1)%n],mat)
        return ri,ro


def make_projectiles():
    meshes=[]
    m=Mesh('rm_mesh_grenade40_lv','40mm grenade launcher projectile / low velocity style','projectile')
    m.add_lathe([(-230,0,'motor'),(-214,58,'motor'),(-158,70,'band'),(-120,70,'band'),(-96,62,'body'),(62,62,'body'),(132,50,'nose'),(204,30,'nose'),(244,0,'tip')],8,math.pi/8)
    meshes.append(m)

    m=Mesh('rm_mesh_m74_flash','M202A1 FLASH M74 66mm rocket visual','projectile')
    m.add_lathe([(-610,0,'motor'),(-585,36,'motor'),(-420,36,'motor'),(-265,42,'band'),(-225,42,'band'),(250,42,'body'),(405,46,'nose'),(545,31,'nose'),(632,0,'tip')],8,0)
    m.add_fins(-585,-430,36,92,6,12,'fin')
    meshes.append(m)

    m=Mesh('rm_mesh_bazooka_m6a3','2.36 inch bazooka M6A3 inspired blunt rocket visual','projectile')
    m.add_lathe([(-500,0,'motor'),(-470,50,'motor'),(-335,54,'motor'),(-285,66,'band'),(-238,66,'band'),(190,66,'body'),(315,72,'nose'),(430,54,'nose'),(480,16,'tip'),(500,0,'tip')],8,math.pi/8)
    m.add_ring_fin(-455,-385,66,116,8,'fin')
    meshes.append(m)

    m=Mesh('rm_mesh_bazooka_m28','3.5 inch Super Bazooka M28/M35 inspired rocket visual','projectile')
    m.add_lathe([(-620,0,'motor'),(-585,42,'motor'),(-390,42,'motor'),(-330,56,'band'),(-285,56,'band'),(-120,58,'body'),(-35,94,'body'),(215,116,'nose'),(405,85,'nose'),(530,28,'tip'),(560,0,'tip')],8,0)
    m.add_fins(-575,-415,42,104,6,11,'fin')
    meshes.append(m)

    m=Mesh('rm_mesh_rpg7_pg7v','RPG-7 PG-7V inspired bulb warhead visual','projectile')
    m.add_lathe([(-675,0,'motor'),(-640,34,'motor'),(-460,34,'motor'),(-405,44,'band'),(-340,44,'band'),(-255,30,'motor'),(-85,30,'motor'),(-10,55,'band'),(92,92,'body'),(230,126,'nose'),(425,98,'nose'),(575,32,'tip'),(620,0,'tip')],8,0)
    m.add_fins(-640,-490,34,92,6,10,'fin')
    meshes.append(m)

    m=Mesh('rm_mesh_rpg7_pg7vr','RPG-7 PG-7VR tandem-inspired two-lobe visual','projectile')
    m.add_lathe([(-735,0,'motor'),(-700,34,'motor'),(-505,34,'motor'),(-445,43,'band'),(-375,43,'band'),(-255,30,'motor'),(-110,30,'motor'),(-32,76,'body'),(130,128,'body'),(280,94,'nose'),(360,32,'band'),(440,60,'nose'),(555,54,'nose'),(655,20,'tip'),(690,0,'tip')],8,math.pi/8)
    m.add_fins(-700,-535,34,94,6,10,'fin')
    meshes.append(m)

    m=Mesh('rm_mesh_rpg7_og7v','RPG-7 OG-7V inspired slim fragmentation projectile visual','projectile')
    m.add_lathe([(-675,0,'motor'),(-640,32,'motor'),(-470,32,'motor'),(-420,42,'band'),(-355,42,'band'),(-200,38,'body'),(260,45,'body'),(440,38,'nose'),(602,18,'tip'),(640,0,'tip')],8,math.pi/8)
    m.add_fins(-640,-495,32,86,6,10,'fin')
    meshes.append(m)

    m=Mesh('rm_mesh_survival_rpg7_cone','survival-horror RPG-7 style cone warhead visual','projectile')
    m.add_lathe([(-760,0,'motor'),(-725,30,'motor'),(-565,30,'motor'),(-505,39,'band'),(-438,39,'band'),(-320,25,'motor'),(-205,25,'motor'),(-150,55,'band'),(-72,112,'body'),(-16,124,'detail'),(68,124,'body'),(128,112,'detail'),(230,90,'nose'),(365,48,'nose'),(500,24,'tip'),(650,18,'tip'),(720,0,'tip')],8,math.pi/8)
    m.add_fins(-715,-565,30,86,6,10,'fin')
    meshes.append(m)

    m=Mesh('rm_mesh_generic_rocket_stub','generic lowpoly rocket stub / fallback','projectile')
    m.add_lathe([(-420,0,'motor'),(-390,42,'motor'),(-240,42,'motor'),(-180,52,'band'),(-142,52,'band'),(210,52,'body'),(360,44,'nose'),(465,0,'tip')],6,0)
    m.add_fins(-385,-260,42,88,4,14,'fin')
    meshes.append(m)
    return meshes

def make_eject_fx():
    meshes=[]
    # 40mm spent case: open front, capped rear primer/rim.
    m=Mesh('rm_mesh_shell_40mm_spent_case','40mm spent case / open brass shell','eject')
    m.add_lathe([(-170,0,'brass'),(-158,72,'brass'),(-132,72,'brass'),(-112,62,'brass'),(118,58,'brass')],8,math.pi/8)
    # front hollow lip, dark inside wall
    r_outer=m.add_ring(126,58,8,math.pi/8)
    r_inner=m.add_ring(126,42,8,math.pi/8)
    m.connect_rings(r_outer,r_inner,'brass')
    r_inner_back=m.add_ring(92,42,8,math.pi/8)
    m.connect_rings(r_inner,r_inner_back,'empty')
    meshes.append(m)

    # A tiny tail cap/debris disk: optional art detail for bazooka-like reload/fire.
    m=Mesh('rm_mesh_debris_bazooka_tail_cap','optional bazooka tail-cap debris disc','eject')
    m.add_tube(-8,8,48,0,8,'detail','detail',phase=math.pi/8,cap_back=True,cap_front=True)
    # add small bent tab
    m.add_box(-6,34,-10,10,42,58,'detail')
    meshes.append(m)

    # Broken seal fragments: two small low-poly flaps that can tumble.
    m=Mesh('rm_mesh_debris_rocket_seal_disc','optional broken rocket seal fragments','eject')
    c=m.add_v(0,0,0)
    a=m.add_v(0,62,0); b=m.add_v(0,18,54); d=m.add_v(0,-8,28); e=m.add_v(0,12,-48); f=m.add_v(0,54,-22)
    m.add_tri(c,a,b,'detail'); m.add_tri(c,b,d,'detail')
    m.add_tri(c,e,f,'detail'); m.add_tri(c,f,a,'detail')
    # slight thickness duplicate
    c2=m.add_v(8,0,0); a2=m.add_v(8,62,0); b2=m.add_v(8,18,54); d2=m.add_v(8,-8,28); e2=m.add_v(8,12,-48); f2=m.add_v(8,54,-22)
    m.add_tri(c2,b2,a2,'detail'); m.add_tri(c2,d2,b2,'detail')
    m.add_tri(c2,f2,e2,'detail'); m.add_tri(c2,a2,f2,'detail')
    meshes.append(m)

    # Four-cell empty rocket clip/cassette for M202-style reload state.
    m=Mesh('rm_mesh_m202_empty_rocket_clip','M202 empty four-rocket clip / cassette visual','eject')
    offs=[(-38,-38),(-38,38),(38,-38),(38,38)]
    for y,z in offs:
        m.add_tube(-150,150,34,25,8,'empty','motor',y0=y,z0=z,phase=math.pi/8)
    # straps/rails to read as one cassette
    m.add_box(-150,150,-86,-72,-74,74,'detail')
    m.add_box(-150,150,72,86,-74,74,'detail')
    m.add_box(-150,150,-74,74,-86,-72,'detail')
    m.add_box(-150,150,-74,74,72,86,'detail')
    m.add_box(-165,-145,-88,88,-88,88,'band')
    m.add_box(145,165,-88,88,-88,88,'band')
    meshes.append(m)

    # Single empty tube cell; useful as weapon-state mouth/hole, not as falling brass.
    m=Mesh('rm_mesh_m202_empty_tube_cell','M202 single empty tube cell state mesh','state')
    m.add_tube(-92,92,50,36,8,'empty','motor',phase=math.pi/8)
    m.add_box(-92,92,-58,-48,-58,58,'detail')
    m.add_box(-92,92,48,58,-58,58,'detail')
    meshes.append(m)

    # RPG tail helper folded fins: attach to projectile while loaded or first frames.
    m=Mesh('rm_mesh_rpg7_tail_folded_fins','RPG-7 tail helper with folded fins','state')
    m.add_lathe([(-190,0,'motor'),(-175,30,'motor'),(165,30,'motor'),(190,0,'motor')],8,math.pi/8)
    # folded strips hugging body
    for i in range(6):
        ang=2*math.pi*i/6.0
        half=math.radians(5)
        r0=31; r1=39
        pts=[]
        for x,r,a in [(-120,r0,ang-half),(-120,r0,ang+half),(80,r1,ang+half),(80,r1,ang-half)]:
            pts.append(m.add_v(x,math.cos(a)*r,math.sin(a)*r))
        m.add_quad(pts[0],pts[1],pts[2],pts[3],'fin')
        m.add_quad(pts[0],pts[3],pts[2],pts[1],'fin')
    meshes.append(m)

    # RPG tail helper open fins: attach or swap after launch.
    m=Mesh('rm_mesh_rpg7_tail_open_fins','RPG-7 tail helper with opened fins','state')
    m.add_lathe([(-190,0,'motor'),(-175,30,'motor'),(165,30,'motor'),(190,0,'motor')],8,math.pi/8)
    m.add_fins(-140,80,30,96,6,11,'fin')
    meshes.append(m)

    # Resident Evil style RPG launcher mouth/empty socket; weapon state, not ejected.
    m=Mesh('rm_mesh_re_rpg7_empty_socket','survival RPG empty launcher socket / dark mouth','state')
    m.add_lathe([(-80,0,'empty'),(-66,92,'empty'),(-22,112,'detail'),(64,98,'empty'),(92,76,'empty')],8,math.pi/8)
    r_outer=m.add_ring(98,76,8,math.pi/8)
    r_inner=m.add_ring(98,52,8,math.pi/8)
    m.connect_rings(r_outer,r_inner,'empty')
    r_back=m.add_ring(40,52,8,math.pi/8)
    m.connect_rings(r_inner,r_back,'motor')
    meshes.append(m)

    # Backblast smoke ring/puff: mesh placeholder for translucent billboard-ish geometry.
    m=Mesh('rm_mesh_fx_backblast_ring','rocket backblast smoke ring mesh helper','fx')
    # three nested annulus discs offset in x for volume; renderer should alpha fade.
    m.add_annulus_disc(-20,50,116,12,'smoke',0)
    m.add_annulus_disc(0,66,148,12,'smoke',math.pi/12)
    m.add_annulus_disc(24,82,178,12,'smoke',0)
    meshes.append(m)

    return meshes

meshes=make_projectiles()+make_eject_fx()

# enum names
ids=[]
for m in meshes:
    ids.append('RMESH_'+m.cname.replace('rm_mesh_','').upper())

# header
h=[]
h.append('/* rocketmeshes.h - tiny static projectile/ejection mesh library. CC0. */')
h.append('/* C89, no heap, no malloc, no floats/doubles in runtime data. */')
h.append('#ifndef ROCKETMESHES_H')
h.append('#define ROCKETMESHES_H')
h.append('')
h.append('#ifdef __cplusplus')
h.append('extern "C" {')
h.append('#endif')
h.append('')
h.append('#define RMESH_Q 8')
h.append('#define RMESH_ONE (1 << RMESH_Q)')
h.append('#define RMESH_NO_MESH (-1)')
h.append('')
h.append('typedef signed short rm_i16;')
h.append('typedef unsigned short rm_u16;')
h.append('typedef unsigned char rm_u8;')
h.append('')
h.append('typedef struct RM_Vertex {')
h.append('    rm_i16 x;')
h.append('    rm_i16 y;')
h.append('    rm_i16 z;')
h.append('} RM_Vertex;')
h.append('')
h.append('typedef struct RM_Tri {')
h.append('    rm_u16 a;')
h.append('    rm_u16 b;')
h.append('    rm_u16 c;')
h.append('    rm_u8 mat;')
h.append('} RM_Tri;')
h.append('')
h.append('typedef struct RM_Mesh {')
h.append('    const char *name;')
h.append('    const RM_Vertex *v;')
h.append('    const RM_Tri *t;')
h.append('    rm_u16 vcount;')
h.append('    rm_u16 tcount;')
h.append('    rm_i16 minx, miny, minz;')
h.append('    rm_i16 maxx, maxy, maxz;')
h.append('} RM_Mesh;')
h.append('')
h.append('typedef struct RM_EjectHint {')
h.append('    rm_i16 fire_fx_mesh;')
h.append('    rm_i16 fire_debris_mesh;')
h.append('    rm_i16 reload_eject_mesh;')
h.append('    rm_i16 state_empty_mesh;')
h.append('    rm_u8 behavior;')
h.append('} RM_EjectHint;')
h.append('')
h.append('enum {')
h.append('    RM_MAT_BODY = 0,')
h.append('    RM_MAT_NOSE = 1,')
h.append('    RM_MAT_BAND = 2,')
h.append('    RM_MAT_FIN = 3,')
h.append('    RM_MAT_MOTOR = 4,')
h.append('    RM_MAT_TIP = 5,')
h.append('    RM_MAT_DETAIL = 6,')
h.append('    RM_MAT_BRASS = 7,')
h.append('    RM_MAT_EMPTY = 8,')
h.append('    RM_MAT_SMOKE = 9,')
h.append('    RM_MAT_COUNT = 10')
h.append('};')
h.append('')
h.append('enum {')
h.append('    RM_EJECT_NONE = 0,')
h.append('    RM_EJECT_BREECH_CASE_ON_RELOAD = 1,')
h.append('    RM_EJECT_ROCKET_SMOKE_ONLY = 2,')
h.append('    RM_EJECT_M202_TUBE_COUNTER = 3,')
h.append('    RM_EJECT_OPTIONAL_TAIL_CAP = 4,')
h.append('    RM_EJECT_STATE_SWAP_ONLY = 5')
h.append('};')
h.append('')
h.append('enum {')
for i,name in enumerate(ids):
    h.append('    %s = %d,' % (name,i))
h.append('    RMESH_COUNT = %d' % len(meshes))
h.append('};')
h.append('')
h.append('const RM_Mesh *rm_get_mesh(int mesh_id);')
h.append('const char *rm_get_mesh_name(int mesh_id);')
h.append('const RM_EjectHint *rm_get_eject_hint(int projectile_mesh_id);')
h.append('')
h.append('#ifdef __cplusplus')
h.append('}')
h.append('#endif')
h.append('')
h.append('#endif')
open(os.path.join(ROOT,'include','rocketmeshes.h'),'w').write('\n'.join(h)+'\n')

# C file
c=[]
c.append('/* rocketmeshes.c - generated static lowpoly projectile/ejection meshes. CC0. */')
c.append('#include "rocketmeshes.h"')
c.append('')
for m in meshes:
    c.append('static const RM_Vertex %s_v[%d] = {' % (m.cname,len(m.v)))
    for i,(x,y,z) in enumerate(m.v):
        c.append('    { %d, %d, %d }%s' % (x,y,z,',' if i<len(m.v)-1 else ''))
    c.append('};')
    c.append('static const RM_Tri %s_t[%d] = {' % (m.cname,len(m.t)))
    for i,(a,b,cc,mat) in enumerate(m.t):
        c.append('    { %d, %d, %d, %d }%s' % (a,b,cc,mat,',' if i<len(m.t)-1 else ''))
    c.append('};')
    minx=min(v[0] for v in m.v); maxx=max(v[0] for v in m.v)
    miny=min(v[1] for v in m.v); maxy=max(v[1] for v in m.v)
    minz=min(v[2] for v in m.v); maxz=max(v[2] for v in m.v)
    c.append('static const RM_Mesh %s = {' % m.cname)
    c.append('    "%s", %s_v, %s_t, %d, %d, %d, %d, %d, %d, %d, %d' % (m.nice,m.cname,m.cname,len(m.v),len(m.t),minx,miny,minz,maxx,maxy,maxz))
    c.append('};')
    c.append('')
c.append('static const RM_Mesh *rm_meshes[RMESH_COUNT] = {')
for i,m in enumerate(meshes):
    c.append('    &%s%s' % (m.cname,',' if i<len(meshes)-1 else ''))
c.append('};')
c.append('')
# Eject hint table for mesh IDs. Need all entries.
def idx(enum_suffix):
    name='RMESH_'+enum_suffix
    return ids.index(name)
NO='RMESH_NO_MESH'
back=idx('FX_BACKBLAST_RING')
case40=idx('SHELL_40MM_SPENT_CASE')
tailcap=idx('DEBRIS_BAZOOKA_TAIL_CAP')
seal=idx('DEBRIS_ROCKET_SEAL_DISC')
clip=idx('M202_EMPTY_ROCKET_CLIP')
tubecell=idx('M202_EMPTY_TUBE_CELL')
refold=idx('RPG7_TAIL_FOLDED_FINS')
reopen=idx('RPG7_TAIL_OPEN_FINS')
resocket=idx('RE_RPG7_EMPTY_SOCKET')
# default for each id
hints=[]
for i in range(len(meshes)):
    hints.append((NO,NO,NO,NO,'RM_EJECT_NONE'))
# Projectiles 0..8
hints[idx('GRENADE40_LV')]=(NO,NO,case40,NO,'RM_EJECT_BREECH_CASE_ON_RELOAD')
hints[idx('M74_FLASH')]=(back,NO,clip,tubecell,'RM_EJECT_M202_TUBE_COUNTER')
hints[idx('BAZOOKA_M6A3')]=(back,tailcap,NO,NO,'RM_EJECT_OPTIONAL_TAIL_CAP')
hints[idx('BAZOOKA_M28')]=(back,seal,NO,NO,'RM_EJECT_OPTIONAL_TAIL_CAP')
hints[idx('RPG7_PG7V')]=(back,NO,NO,reopen,'RM_EJECT_ROCKET_SMOKE_ONLY')
hints[idx('RPG7_PG7VR')]=(back,NO,NO,reopen,'RM_EJECT_ROCKET_SMOKE_ONLY')
hints[idx('RPG7_OG7V')]=(back,NO,NO,reopen,'RM_EJECT_ROCKET_SMOKE_ONLY')
hints[idx('SURVIVAL_RPG7_CONE')]=(back,NO,NO,resocket,'RM_EJECT_ROCKET_SMOKE_ONLY')
hints[idx('GENERIC_ROCKET_STUB')]=(back,NO,NO,NO,'RM_EJECT_ROCKET_SMOKE_ONLY')
# state swap helpers
hints[refold]=(NO,NO,NO,reopen,'RM_EJECT_STATE_SWAP_ONLY')
hints[reopen]=(NO,NO,NO,NO,'RM_EJECT_STATE_SWAP_ONLY')

c.append('static const RM_EjectHint rm_eject_hints[RMESH_COUNT] = {')
for i,(fx,deb,rel,state,beh) in enumerate(hints):
    c.append('    { %s, %s, %s, %s, %s }%s' % (str(fx),str(deb),str(rel),str(state),beh,',' if i<len(hints)-1 else ''))
c.append('};')
c.append('')
c.append('const RM_Mesh *rm_get_mesh(int mesh_id)')
c.append('{')
c.append('    if (mesh_id < 0 || mesh_id >= RMESH_COUNT) {')
c.append('        return (const RM_Mesh *)0;')
c.append('    }')
c.append('    return rm_meshes[mesh_id];')
c.append('}')
c.append('')
c.append('const char *rm_get_mesh_name(int mesh_id)')
c.append('{')
c.append('    const RM_Mesh *m;')
c.append('    m = rm_get_mesh(mesh_id);')
c.append('    if (!m) {')
c.append('        return "";')
c.append('    }')
c.append('    return m->name;')
c.append('}')
c.append('')
c.append('const RM_EjectHint *rm_get_eject_hint(int projectile_mesh_id)')
c.append('{')
c.append('    if (projectile_mesh_id < 0 || projectile_mesh_id >= RMESH_COUNT) {')
c.append('        return (const RM_EjectHint *)0;')
c.append('    }')
c.append('    return &rm_eject_hints[projectile_mesh_id];')
c.append('}')
open(os.path.join(ROOT,'src','rocketmeshes.c'),'w').write('\n'.join(c)+'\n')

# OBJ dumper
obj_c=r'''/* rocketmeshes_dump_obj.c - test exporter. C89; no heap. */
#include <stdio.h>
#include <stdlib.h>
#include "rocketmeshes.h"

static void print_q8(FILE *fp, int v)
{
    int neg;
    int whole;
    int rem;
    neg = 0;
    if (v < 0) {
        neg = 1;
        v = -v;
    }
    whole = v >> RMESH_Q;
    rem = ((v & (RMESH_ONE - 1)) * 1000) >> RMESH_Q;
    if (neg) {
        fputc('-', fp);
    }
    fprintf(fp, "%d.%03d", whole, rem);
}

static void write_obj(const RM_Mesh *m, FILE *fp)
{
    unsigned int i;
    unsigned int last_mat;
    fprintf(fp, "# rocketmeshes OBJ dump: %s\n", m->name);
    fprintf(fp, "# vertices=%u triangles=%u scale=Q%d\n", (unsigned int)m->vcount, (unsigned int)m->tcount, RMESH_Q);
    fprintf(fp, "o mesh_%s\n", m->name);
    for (i = 0; i < (unsigned int)m->vcount; ++i) {
        fputs("v ", fp);
        print_q8(fp, m->v[i].x); fputc(' ', fp);
        print_q8(fp, m->v[i].y); fputc(' ', fp);
        print_q8(fp, m->v[i].z); fputc('\n', fp);
    }
    last_mat = 999u;
    for (i = 0; i < (unsigned int)m->tcount; ++i) {
        if ((unsigned int)m->t[i].mat != last_mat) {
            last_mat = (unsigned int)m->t[i].mat;
            fprintf(fp, "usemtl rm_mat_%u\n", last_mat);
        }
        fprintf(fp, "f %u %u %u\n", (unsigned int)m->t[i].a + 1u, (unsigned int)m->t[i].b + 1u, (unsigned int)m->t[i].c + 1u);
    }
}

int main(int argc, char **argv)
{
    int id;
    const RM_Mesh *m;
    FILE *fp;

    if (argc < 3) {
        fprintf(stderr, "usage: rocketmeshes_dump_obj <mesh_id> <out.obj>\n");
        fprintf(stderr, "mesh ids: 0..%d\n", RMESH_COUNT - 1);
        return 2;
    }
    id = atoi(argv[1]);
    m = rm_get_mesh(id);
    if (!m) {
        fprintf(stderr, "bad mesh id\n");
        return 2;
    }
    fp = fopen(argv[2], "w");
    if (!fp) {
        fprintf(stderr, "could not write obj\n");
        return 1;
    }
    write_obj(m, fp);
    fclose(fp);
    return 0;
}
'''
open(os.path.join(ROOT,'tools','rocketmeshes_dump_obj.c'),'w').write(obj_c)

ex=r'''/* rocketmeshes_example.c - minimal use/eject-hint example. */
#include <stdio.h>
#include "rocketmeshes.h"

static void draw_mesh_stub(const RM_Mesh *m)
{
    printf("draw %s: %u vertices, %u triangles\n", m->name, (unsigned int)m->vcount, (unsigned int)m->tcount);
}

static void show_eject_hint(int mesh_id)
{
    const RM_EjectHint *h;
    h = rm_get_eject_hint(mesh_id);
    if (!h) {
        return;
    }
    printf("eject hint for %s\n", rm_get_mesh_name(mesh_id));
    printf("  behavior=%u\n", (unsigned int)h->behavior);
    printf("  fire_fx_mesh=%d\n", (int)h->fire_fx_mesh);
    printf("  fire_debris_mesh=%d\n", (int)h->fire_debris_mesh);
    printf("  reload_eject_mesh=%d\n", (int)h->reload_eject_mesh);
    printf("  state_empty_mesh=%d\n", (int)h->state_empty_mesh);
}

int main(void)
{
    const RM_Mesh *m;
    m = rm_get_mesh(RMESH_SURVIVAL_RPG7_CONE);
    if (m) {
        draw_mesh_stub(m);
        show_eject_hint(RMESH_SURVIVAL_RPG7_CONE);
    }
    m = rm_get_mesh(RMESH_SHELL_40MM_SPENT_CASE);
    if (m) {
        draw_mesh_stub(m);
    }
    return 0;
}
'''
open(os.path.join(ROOT,'examples','rocketmeshes_example.c'),'w').write(ex)

mk=r'''CC=gcc
CFLAGS=-std=c89 -pedantic -Wall -Wextra -Iinclude

all: rocketmeshes_example rocketmeshes_dump_obj

rocketmeshes_example: examples/rocketmeshes_example.c src/rocketmeshes.c include/rocketmeshes.h
	$(CC) $(CFLAGS) examples/rocketmeshes_example.c src/rocketmeshes.c -o rocketmeshes_example

rocketmeshes_dump_obj: tools/rocketmeshes_dump_obj.c src/rocketmeshes.c include/rocketmeshes.h
	$(CC) $(CFLAGS) tools/rocketmeshes_dump_obj.c src/rocketmeshes.c -o rocketmeshes_dump_obj

obj: rocketmeshes_dump_obj
	./rocketmeshes_dump_obj 0 obj/mesh_0.obj
	./rocketmeshes_dump_obj 1 obj/mesh_1.obj
	./rocketmeshes_dump_obj 2 obj/mesh_2.obj
	./rocketmeshes_dump_obj 3 obj/mesh_3.obj
	./rocketmeshes_dump_obj 4 obj/mesh_4.obj
	./rocketmeshes_dump_obj 5 obj/mesh_5.obj
	./rocketmeshes_dump_obj 6 obj/mesh_6.obj
	./rocketmeshes_dump_obj 7 obj/mesh_7.obj
	./rocketmeshes_dump_obj 8 obj/mesh_8.obj
	./rocketmeshes_dump_obj 9 obj/mesh_9.obj
	./rocketmeshes_dump_obj 10 obj/mesh_10.obj
	./rocketmeshes_dump_obj 11 obj/mesh_11.obj
	./rocketmeshes_dump_obj 12 obj/mesh_12.obj
	./rocketmeshes_dump_obj 13 obj/mesh_13.obj
	./rocketmeshes_dump_obj 14 obj/mesh_14.obj
	./rocketmeshes_dump_obj 15 obj/mesh_15.obj
	./rocketmeshes_dump_obj 16 obj/mesh_16.obj
	./rocketmeshes_dump_obj 17 obj/mesh_17.obj

clean:
	rm -f rocketmeshes_example rocketmeshes_dump_obj obj/*.obj
'''
open(os.path.join(ROOT,'Makefile'),'w').write(mk)

# README
readme=[]
readme.append('# rocketmeshes')
readme.append('')
readme.append('Low-poly static projectile + ejection/state helper mesh library for C89 game engines.')
readme.append('')
readme.append('Design target:')
readme.append('')
readme.append('- C89 friendly')
readme.append('- no malloc / realloc / free')
readme.append('- no heap ownership')
readme.append('- no float / double in runtime mesh data')
readme.append('- fixed-point-ish coordinates: `RMESH_Q = 8`, so 256 units = 1 mesh unit')
readme.append('- triangle materials are small IDs only')
readme.append('- purely visual dummy meshes for games/tools; not real engineering dimensions')
readme.append('')
readme.append('## Main rule for realism')
readme.append('')
readme.append('- `RMESH_GRENADE40_LV`: use `RMESH_SHELL_40MM_SPENT_CASE` on reload/open-breech.')
readme.append('- M202/Bazooka/RPG meshes: do not spawn brass shells per shot; use backblast/smoke, optional tiny debris, empty tube/clip state, or fin-state swap.')
readme.append('')
readme.append('## Mesh IDs')
readme.append('')
readme.append('| ID | Enum | Group | Mesh | V | Tri |')
readme.append('|---:|---|---|---|---:|---:|')
for i,m in enumerate(meshes):
    readme.append('| %d | `%s` | %s | %s | %d | %d |' % (i,ids[i],m.group,m.nice,len(m.v),len(m.t)))
readme.append('')
readme.append('## Ejection hint table')
readme.append('')
readme.append('`rm_get_eject_hint(projectile_mesh_id)` returns:')
readme.append('')
readme.append('```c')
readme.append('typedef struct RM_EjectHint {')
readme.append('    rm_i16 fire_fx_mesh;       /* usually RMESH_FX_BACKBLAST_RING */')
readme.append('    rm_i16 fire_debris_mesh;   /* optional tiny debris, or -1 */')
readme.append('    rm_i16 reload_eject_mesh;  /* e.g. 40mm shell or M202 empty clip */')
readme.append('    rm_i16 state_empty_mesh;   /* visual state helper, not a falling shell */')
readme.append('    rm_u8 behavior;')
readme.append('} RM_EjectHint;')
readme.append('```')
readme.append('')
readme.append('Suggested behavior mapping:')
readme.append('')
readme.append('| Behavior | Meaning |')
readme.append('|---:|---|')
readme.append('| `RM_EJECT_NONE` | no automatic helper |')
readme.append('| `RM_EJECT_BREECH_CASE_ON_RELOAD` | spawn spent case only when opening/reloading |')
readme.append('| `RM_EJECT_ROCKET_SMOKE_ONLY` | rocket leaves launcher; smoke/backblast but no shell |')
readme.append('| `RM_EJECT_M202_TUBE_COUNTER` | mark fired tube empty; spawn empty clip only on full reload |')
readme.append('| `RM_EJECT_OPTIONAL_TAIL_CAP` | smoke plus small art-only debris |')
readme.append('| `RM_EJECT_STATE_SWAP_ONLY` | helper mesh for folded/open fin state |')
readme.append('')
readme.append('## Minimal use')
readme.append('')
readme.append('```c')
readme.append('#include "rocketmeshes.h"')
readme.append('')
readme.append('const RM_Mesh *m = rm_get_mesh(RMESH_RPG7_PG7V);')
readme.append('const RM_EjectHint *h = rm_get_eject_hint(RMESH_RPG7_PG7V);')
readme.append('')
readme.append('/* submit m->v and m->t to your own renderer */')
readme.append('/* h->fire_fx_mesh gives you the backblast helper, if any */')
readme.append('```')
readme.append('')
readme.append('## Build test tools')
readme.append('')
readme.append('```sh')
readme.append('make')
readme.append('./rocketmeshes_example')
readme.append('./rocketmeshes_dump_obj 7 obj/survival_rpg7.obj')
readme.append('```')
readme.append('')
readme.append('## Material IDs')
readme.append('')
readme.append('```c')
for i,n in enumerate(['BODY','NOSE','BAND','FIN','MOTOR','TIP','DETAIL','BRASS','EMPTY','SMOKE']):
    readme.append('RM_MAT_%s = %d' % (n,i))
readme.append('```')
readme.append('')
open(os.path.join(ROOT,'README.md'),'w').write('\n'.join(readme)+'\n')

license_text='''CC0 1.0 Universal

This generated source package is released as CC0/public-domain-style material.
You may copy, modify, integrate, and redistribute it without attribution.
'''
open(os.path.join(ROOT,'LICENSE_CC0.txt'),'w').write(license_text)

meta=[]
for i,m in enumerate(meshes):
    meta.append({'id':i,'enum':ids[i],'cname':m.cname,'nice':m.nice,'group':m.group,'v':m.v,'t':m.t})
open(os.path.join(ROOT,'meshdata.json'),'w').write(json.dumps(meta))
print('generated',len(meshes),'meshes')
