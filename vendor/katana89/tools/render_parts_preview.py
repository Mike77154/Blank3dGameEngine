from pathlib import Path
import math
import numpy as np
import trimesh
import matplotlib.pyplot as plt
from matplotlib.collections import PolyCollection
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'preview'
OUT.mkdir(exist_ok=True)
FILES=sorted((ROOT/'generated').glob('*.obj'))


def collect(path, mode):
    scene=trimesh.load(path,force='scene',process=False)
    polys=[]; cols=[]; depths=[]; uvall=[]
    zmax=max(g.bounds[1,2] for g in scene.geometry.values())
    for name,g in scene.geometry.items():
        v=np.asarray(g.vertices,float); f=np.asarray(g.faces,int)
        base=np.array(g.visual.material.main_color[:3],float)/255.0
        for face in f:
            tri=v[face]
            cz=float(np.mean(tri[:,2]))
            if mode=='guard':
                if not (name in ('mat_3','mat_8') and np.max(np.abs(tri[:,2]))<2.0): continue
                uv=np.column_stack([tri[:,0],tri[:,1]])
                depth=float(np.mean(tri[:,2]))
            elif mode=='handle':
                if cz>2.2: continue
                uv=np.column_stack([-tri[:,2]+tri[:,0]*0.20,tri[:,1]+tri[:,0]*0.60])
                depth=float(np.mean(tri[:,0]))
            else:
                if np.min(tri[:,2])<zmax*0.72: continue
                uv=np.column_stack([tri[:,2],tri[:,1]+tri[:,0]*0.55])
                depth=float(np.mean(tri[:,0]))
            n=np.cross(tri[1]-tri[0],tri[2]-tri[0]); nl=np.linalg.norm(n)
            shade=0.86 if nl<1e-8 else 0.68+0.30*abs(float(n[2 if mode=='guard' else 0]/nl))
            polys.append(uv); cols.append(np.clip(base*shade+0.04,0,1)); depths.append(depth); uvall.append(uv)
    order=np.argsort(depths)
    return [polys[i] for i in order],[cols[i] for i in order],np.vstack(uvall)


def render(path,mode,title,outpath):
    polys,cols,uv=collect(path,mode)
    minx,miny=uv.min(0); maxx,maxy=uv.max(0)
    dx=maxx-minx;dy=maxy-miny
    fig=plt.figure(figsize=(4.8,3.0),dpi=150); ax=fig.add_axes([0,0,1,1])
    fig.patch.set_facecolor('#111318'); ax.set_facecolor('#111318')
    ax.add_collection(PolyCollection(polys,facecolors=cols,edgecolors='none',linewidths=0))
    ax.set_xlim(minx-max(0.5,dx*0.10),maxx+max(0.5,dx*0.10)); ax.set_ylim(miny-max(0.5,dy*0.14),maxy+max(0.5,dy*0.14))
    ax.set_aspect('equal'); ax.axis('off')
    ax.text(minx,maxy+max(0.35,dy*0.09),title,color='white',fontsize=11,weight='bold')
    fig.savefig(outpath,facecolor=fig.get_facecolor(),dpi=150);plt.close(fig)

cards=[]
items=[
 ('guard',0,'Maru tsuba'),('guard',3,'Mokko + sukashi cue'),('guard',13,'Aoi tsuba'),
 ('guard',21,'Nadekaku tsuba'),('guard',23,'Hachi tsuba'),('guard',26,'Cross war tsuba'),
 ('handle',0,'Rikko tsuka'),('handle',4,'Straight tsuka'),('handle',6,'Ha-agari tsuka'),
 ('tip',5,'Ko-kissaki'),('tip',0,'Chu-kissaki'),('tip',6,'O-kissaki')]
for i,(mode,idx,title) in enumerate(items):
    p=OUT/f'part_{i:02d}.png';render(FILES[idx],mode,title,p);cards.append(Image.open(p).convert('RGB'))

cw=max(x.width for x in cards);ch=max(x.height for x in cards);cols=3;rows=4;margin=18;header=105
canvas=Image.new('RGB',(cols*cw+(cols+1)*margin,rows*ch+(rows+1)*margin+header),(12,14,18))
d=ImageDraw.Draw(canvas)
try:
 font1=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',34)
 font2=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',17)
except: font1=font2=None
d.text((margin,22),'katana89 v1.1 — rebuilt details',fill=(238,242,247),font=font1)
d.text((margin,68),'open tsuba, crossed tsukamaki and corrected kissaki proportions',fill=(157,167,182),font=font2)
for i,img in enumerate(cards):
 x=margin+(i%cols)*(cw+margin);y=header+margin+(i//cols)*(ch+margin);canvas.paste(img,(x,y))
out=OUT/'katana89_parts_preview.png';canvas.save(out,quality=95);print(out)
