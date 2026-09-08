"""Render certificate triangle data for offline geometry inspection (no engine UI)."""
import sys
import numpy as np
from PIL import Image, ImageDraw

data=np.loadtxt(sys.argv[1]); tri=data[:,1:].reshape(-1,3,3)
width,height=1100,760
eye=np.array([4.6,-4.5,3.3]); target=np.array([1.4,.65,.7])
if len(sys.argv)>3:eye=np.array([1.9,-1.3,1.3]);target=np.array([1.43,.5,.85])
forward=target-eye;forward/=np.linalg.norm(forward)
right=np.cross(forward,[0,0,1]);right/=np.linalg.norm(right)
up=np.cross(right,forward)
relative=tri-eye
depth=relative@forward
screen=np.stack([width*.5+(relative@right)/depth*850,height*.5-(relative@up)/depth*850],axis=-1)
pixels=np.full((height,width,3),[201,214,225],dtype=np.uint8)
zbuffer=np.full((height,width),np.inf)
light=np.array([-.3,-.5,.8]);light/=np.linalg.norm(light)
for i,(p,z) in enumerate(zip(screen,depth)):
    if np.any(z<=.01): continue
    x0=max(0,int(np.floor(p[:,0].min())));x1=min(width-1,int(np.ceil(p[:,0].max())))
    y0=max(0,int(np.floor(p[:,1].min())));y1=min(height-1,int(np.ceil(p[:,1].max())))
    if x0>x1 or y0>y1:continue
    det=(p[1,1]-p[2,1])*(p[0,0]-p[2,0])+(p[2,0]-p[1,0])*(p[0,1]-p[2,1])
    if abs(det)<1e-9:continue
    yy,xx=np.mgrid[y0:y1+1,x0:x1+1];xx=xx+.5;yy=yy+.5
    a=((p[1,1]-p[2,1])*(xx-p[2,0])+(p[2,0]-p[1,0])*(yy-p[2,1]))/det
    b=((p[2,1]-p[0,1])*(xx-p[2,0])+(p[0,0]-p[2,0])*(yy-p[2,1]))/det
    c=1-a-b
    with np.errstate(divide='ignore',invalid='ignore'):depths=1/(a/z[0]+b/z[1]+c/z[2])
    region=zbuffer[y0:y1+1,x0:x1+1];mask=(a>=-1e-8)&(b>=-1e-8)&(c>=-1e-8)&(depths<region)
    normal=np.cross(tri[i,1]-tri[i,0],tri[i,2]-tri[i,0]);normal/=max(np.linalg.norm(normal),1e-20)
    brightness=.35+.65*max(0,np.dot(normal,light))
    color=np.array([160,156,151] if data[i,0]==0 else [152,112,75])*brightness
    pixels[y0:y1+1,x0:x1+1][mask]=color.astype(np.uint8);region[mask]=depths[mask]
im=Image.fromarray(pixels);draw=ImageDraw.Draw(im)
draw.text((20,20),'OFFLINE GEOMETRY CHECK - not an editor screenshot',fill='black')
im.save(sys.argv[2])
