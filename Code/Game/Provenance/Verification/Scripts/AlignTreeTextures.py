"""Mechanical runtime texture conversion: GPU block dimensions, sources retained.
Resample non-power-of-two foliage to 1024-square for block-safe mip uploads.
Normalized UV coverage and the entire image are retained (no crop).
"""
import bpy
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
DATA=ROOT.parents[2]/'Data/Provenance/TreeTrial/v002'
DEST=ROOT/'output/integration/tree_trial_v002/aligned_textures_pow2'
DEST.mkdir(exist_ok=True)
report=[]
for p in DATA.glob('*.png'):
    image=bpy.data.images.load(str(p),check_existing=False)
    w,h=image.size
    if (w & (w-1)) or (h & (h-1)):
        target=DEST/p.name
        if target.exists():raise RuntimeError('Existing derivative '+str(target))
        image.scale(1024,1024)
        image.filepath_raw=str(target);image.file_format='PNG';image.save()
        report.append(dict(source=str(p),target=str(target),before=[w,h],after=list(image.size)))
    bpy.data.images.remove(image)
(DEST/'report.json').write_text(json.dumps(report,indent=2))
print('ALIGNED_TEXTURES',json.dumps(report))
