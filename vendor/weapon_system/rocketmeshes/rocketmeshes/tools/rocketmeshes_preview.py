import json, math, os
from PIL import Image, ImageDraw, ImageFont
ROOT='/mnt/data/rocketmeshes'
meta=json.load(open(os.path.join(ROOT,'meshdata.json')))
COLORS={
  0:(86,92,96),    # body
  1:(120,128,130), # nose
  2:(170,132,72),  # band
  3:(58,64,68),    # fin
  4:(70,75,80),    # motor
  5:(190,190,180), # tip
  6:(150,150,150), # detail
  7:(178,132,62),  # brass
  8:(32,34,36),    # empty/dark
  9:(198,202,202), # smoke
}
EDGE=(25,25,25)
BG=(238,238,232)

def project(v, scale=0.42, ox=0, oy=0):
    x,y,z=v
    px = x*0.86 + y*0.38
    py = -z*0.95 + x*0.18 - y*0.18
    return (ox+px*scale, oy+py*scale)

def render_mesh(m, W=760, H=240, title=True):
    img=Image.new('RGB',(W,H),BG)
    d=ImageDraw.Draw(img)
    v=m['v']; tris=m['t']
    pts=[project(p,1,0,0) for p in v]
    minx=min(p[0] for p in pts); maxx=max(p[0] for p in pts)
    miny=min(p[1] for p in pts); maxy=max(p[1] for p in pts)
    margin=25
    margin_top=45 if title else 20
    sx=(W-2*margin)/(maxx-minx) if maxx>minx else 1
    sy=(H-margin-margin_top)/(maxy-miny) if maxy>miny else 1
    scale=min(sx,sy)
    ox=margin - minx*scale + ((W-2*margin)-(maxx-minx)*scale)/2
    oy=margin_top - miny*scale + ((H-margin-margin_top)-(maxy-miny)*scale)/2
    def P(i): return project(v[i],scale,ox,oy)
    order=[]
    for idx,t in enumerate(tris):
        a,b,c,mat=t
        depth=(v[a][0]+v[b][0]+v[c][0])*0.25 + (v[a][1]+v[b][1]+v[c][1])*0.18 + (v[a][2]+v[b][2]+v[c][2])*0.08
        order.append((depth,idx))
    for _,idx in sorted(order):
        a,b,c,mat=tris[idx]
        poly=[P(a),P(b),P(c)]
        col=COLORS.get(mat,(100,100,100))
        d.polygon(poly, fill=col, outline=EDGE)
    if title:
        try:
            font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',15)
            small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',13)
        except Exception:
            font=None; small=None
        d.text((18,8),m['enum'],fill=(20,20,20),font=small)
        d.text((18,24),m['nice'],fill=(20,20,20),font=font)
        d.text((W-168,14),'v:%d  tri:%d'%(len(v),len(tris)),fill=(50,50,50),font=small)
    return img

# clear previews for stale old naming? keep okay but overwrite same directory
os.makedirs(os.path.join(ROOT,'previews'),exist_ok=True)
for i,m in enumerate(meta):
    img=render_mesh(m)
    img.save(os.path.join(ROOT,'previews','preview_%02d_%s.png'%(i,m['cname'].replace('rm_mesh_',''))))

# full sheet
thumbs=[render_mesh(m,560,184,True) for m in meta]
cols=2; rows=(len(thumbs)+cols-1)//cols
sheet=Image.new('RGB',(cols*560,rows*184),BG)
for i,img in enumerate(thumbs):
    sheet.paste(img,((i%cols)*560,(i//cols)*184))
sheet.save(os.path.join(ROOT,'previews','rocketmeshes_sheet.png'))

# ejection/fx sheet only
fx=[m for m in meta if m.get('group')!='projectile']
thumbs=[render_mesh(m,560,184,True) for m in fx]
cols=2; rows=(len(thumbs)+cols-1)//cols
sheet=Image.new('RGB',(cols*560,rows*184),BG)
for i,img in enumerate(thumbs):
    sheet.paste(img,((i%cols)*560,(i//cols)*184))
sheet.save(os.path.join(ROOT,'previews','rocketmeshes_eject_fx_sheet.png'))

# gameplay rule mini sheet: projectile vs eject helpers
sel_ids=[0,9,1,12,2,10,7,16,17]
thumbs=[render_mesh(meta[i],460,160,True) for i in sel_ids]
cols=3; rows=(len(thumbs)+cols-1)//cols
sheet=Image.new('RGB',(cols*460,rows*160),BG)
for i,img in enumerate(thumbs):
    sheet.paste(img,((i%cols)*460,(i//cols)*160))
sheet.save(os.path.join(ROOT,'previews','rocketmeshes_realism_rules_sheet.png'))
print('previews generated')
