from pathlib import Path
from PIL import Image
import numpy as np
P=Path(__file__).resolve().parent/'textures';P.mkdir(parents=True,exist_ok=True)
rng=np.random.default_rng(89231);n=1024;y,x=np.mgrid[:n,:n]/n
for stem,base in [('iron',[57,58,59]),('brass',[148,111,57]),('wood',[101,61,31]),('leather',[45,30,23])]:
    grain=np.zeros((n,n))
    for k in range(28):
        fx=rng.integers(5,100);fy=rng.integers(2,70) if stem!='wood' else rng.integers(1,5)
        grain+=np.sin(2*np.pi*(x*fx+y*fy)+rng.uniform(0,7))/(6+k)
    fine=rng.normal(0,.055,(n,n));v=grain*.18+fine
    if stem=='wood':v+=np.sin(2*np.pi*x*62+np.sin(y*15)*1.5)*.12
    a=np.clip(np.array(base)[None,None,:]*(1+v[:,:,None]),0,255).astype('uint8');Image.fromarray(a).save(P/(stem+'_color.png'))
    dy,dx=np.gradient(v);normal=np.stack((-dx*1.2,-dy*1.2,np.ones_like(v)),2);normal/=np.linalg.norm(normal,axis=2)[:,:,None]
    Image.fromarray(np.uint8(np.clip((normal*.5+.5)*255,0,255))).save(P/(stem+'_normal.png'))
print('Four authored surface texture pairs prepared.')
