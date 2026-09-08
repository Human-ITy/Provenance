"""Deterministic native surface textures for the modeled starter tools."""
from pathlib import Path
from PIL import Image,ImageFilter
import numpy as np
P=Path(__file__).resolve().parent/'textures';P.mkdir(parents=True,exist_ok=True)
rng=np.random.default_rng(27182);n=1024;y,x=np.mgrid[:n,:n]/n
def noise(size):
    im=Image.fromarray(np.uint8(rng.random((size,size))*255))
    return np.asarray(im.resize((n,n),Image.Resampling.BICUBIC)).astype(float)/255-.5
coarse=noise(10);mid=noise(45);fine=noise(220)
for stem,base in [('stone',[45,47,50]),('wood',[91,54,26]),('leather',[36,18,12]),('fiber',[132,97,55]),('accent',[69,84,33])]:
    v=coarse*.5+mid*.27+fine*.16+rng.normal(0,.023,(n,n))
    if stem=='stone':
        # Mineral facets and fine seams in a matte charcoal stone.
        first=np.full((n,n),99.);second=first.copy()
        for _ in range(72):
            cx,cy=rng.random(2);d=(x-cx)**2+(y-cy)**2
            closer=d<first;second=np.where(closer,first,np.minimum(second,d));first=np.minimum(first,d)
        seams=np.exp(-np.maximum(0,second-first)*2200)
        v=coarse*.55+mid*.65+fine*.55+seams*.12-np.exp(-first*350)*.12
    elif stem=='wood':
        phase=x*48+np.sin(y*6+x*9)*.35+coarse*.8
        # Longitudinal grain with tapered dark fibres and occasional knots.
        grain=np.sin(phase*2*np.pi);v+=grain*.12-np.maximum(0,grain-.55)*.46
        for cx,cy in [(.23,.33),(.72,.76)]:
            d=np.sqrt(((x-cx)*2.5)**2+((y-cy)*.45)**2)
            v+=np.sin(d*270)*np.exp(-d*25)*.18-np.exp(-d*80)*.35
    elif stem=='fiber':v+=np.sin(x*2*np.pi*150+np.sin(y*12))*.08
    elif stem=='leather':v=coarse*.3+mid*.25+fine*.3+np.maximum(0,noise(400))*.2
    color=np.clip(np.array(base)[None,None,:]*(1+v[:,:,None]),0,255).astype('uint8')
    Image.fromarray(color).save(P/(stem+'_color.png'))
    dy,dx=np.gradient(v);strength=2.4 if stem=='stone' else 1.4
    normal=np.stack((-dx*strength,-dy*strength,np.ones_like(v)),2);normal/=np.linalg.norm(normal,axis=2)[:,:,None]
    Image.fromarray(np.uint8(np.clip((normal*.5+.5)*255,0,255))).save(P/(stem+'_normal.png'))
print('Starter tool surface textures complete.')
