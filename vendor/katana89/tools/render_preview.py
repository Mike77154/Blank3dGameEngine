from pathlib import Path
import math
import numpy as np
import trimesh
import matplotlib.pyplot as plt
from matplotlib.collections import PolyCollection
from PIL import Image, ImageOps, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'preview'
OUT.mkdir(exist_ok=True)

SELECT = [0,2,3,8,9,12,13,16,20,24,26,28]
FILES = sorted((ROOT/'generated').glob('*.obj'))


def render_obj(path: Path, out_path: Path):
    scene = trimesh.load(path, force='scene', process=False)
    polys = []
    colors = []
    depths = []
    all_uv = []
    light = np.array([0.55, -0.25, 0.80], dtype=float)
    light /= np.linalg.norm(light)

    for name, mesh in scene.geometry.items():
        verts = np.asarray(mesh.vertices, dtype=float)
        faces = np.asarray(mesh.faces, dtype=int)
        if len(verts) == 0 or len(faces) == 0:
            continue
        base = np.array(getattr(mesh.visual.material, 'main_color', [160,160,160,255])[:3], dtype=float)/255.0
        for f in faces:
            tri = verts[f]
            # Orthographic gameplay-ish beauty angle: length horizontal, curvature vertical.
            u = tri[:,2] + tri[:,0]*0.33
            v = tri[:,1] + tri[:,0]*0.55
            uv = np.column_stack([u,v])
            n = np.cross(tri[1]-tri[0], tri[2]-tri[0])
            nl = np.linalg.norm(n)
            shade = 0.78
            if nl > 1e-9:
                n /= nl
                shade = 0.68 + 0.32*abs(float(np.dot(n,light)))
            col = np.clip(base*shade + 0.04, 0, 1)
            polys.append(uv)
            colors.append(col)
            depths.append(float(np.mean(tri[:,0]) - np.mean(tri[:,1])*0.03))
            all_uv.append(uv)

    order = np.argsort(depths)
    polys = [polys[i] for i in order]
    colors = [colors[i] for i in order]
    uv_all = np.vstack(all_uv)
    minx,miny = uv_all.min(axis=0)
    maxx,maxy = uv_all.max(axis=0)
    padx = max(3.0,(maxx-minx)*0.025)
    pady = max(1.8,(maxy-miny)*0.16)

    fig = plt.figure(figsize=(9.2,2.7), dpi=160)
    ax = fig.add_axes([0,0,1,1])
    ax.set_facecolor('#111318')
    fig.patch.set_facecolor('#111318')

    # Soft ground shadow, using silhouette-ish offset.
    shadow_polys=[]
    for p in polys:
        q=p.copy(); q[:,1]=miny-pady*0.10 + (q[:,1]-miny)*0.06; q[:,0]+=1.2
        shadow_polys.append(q)
    ax.add_collection(PolyCollection(shadow_polys, facecolors=(0,0,0,0.18), edgecolors='none'))
    ax.add_collection(PolyCollection(polys, facecolors=colors, edgecolors='none', linewidths=0))
    ax.set_xlim(minx-padx,maxx+padx)
    ax.set_ylim(miny-pady,maxy+pady)
    ax.set_aspect('equal', adjustable='box')
    ax.axis('off')
    title = path.stem.split('_',1)[1].replace('_',' ').title()
    ax.text(minx, maxy+pady*0.48, title, color='white', fontsize=11, weight='bold', va='center')
    ax.text(maxx, maxy+pady*0.48, 'C89 · FIXED POINT · LOW POLY', color='#9da7b6', fontsize=7.5, ha='right', va='center')
    fig.savefig(out_path, facecolor=fig.get_facecolor(), dpi=160)
    plt.close(fig)


cards=[]
for idx in SELECT:
    p=FILES[idx]
    out=OUT/f'card_{idx:02d}.png'
    render_obj(p,out)
    cards.append(Image.open(out).convert('RGB'))

cell_w=max(i.width for i in cards)
cell_h=max(i.height for i in cards)
cols=2
rows=math.ceil(len(cards)/cols)
margin=22
header=120
canvas=Image.new('RGB',(cols*cell_w+(cols+1)*margin, rows*cell_h+(rows+1)*margin+header),(12,14,18))
draw=ImageDraw.Draw(canvas)
try:
    font_big=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',36)
    font_small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',18)
except Exception:
    font_big=font_small=None

draw.text((margin,26),'katana89 v1.1 — realistic low-poly collection',fill=(238,242,247),font=font_big)
draw.text((margin,76),'12 of 33 presets · rebuilt silhouettes, fittings and kissaki topology',fill=(157,167,182),font=font_small)
for i,img in enumerate(cards):
    x=margin+(i%cols)*(cell_w+margin)
    y=header+margin+(i//cols)*(cell_h+margin)
    canvas.paste(img,(x,y))

preview=OUT/'katana89_collection_preview.png'
canvas.save(preview,quality=95)
print(preview)
