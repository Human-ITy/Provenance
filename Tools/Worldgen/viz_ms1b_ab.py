"""MS1.B A/B contact sheet.

TOP: real-renderer near/mid origin world (MV1), MS1.B OFF (frozen single-layer
DIAGNOSTIC palette -- the 'alpine white' / olive / grey the cut retires) vs ON
(one shared SurfaceState->appearance resolver; material-derived).

BOTTOM: the far origin world (+/-160 km) rendered top-down through the SAME MS1.B
appearance palette from the SHIPPED page descriptor -- volcanic (dark), rock,
regolith, sediment provinces read as different MATERIALS, not an elevation ramp.
This bottom panel mirrors the C++ Ms1::ResolveAppearance base-albedo tables so it
previews the far material world the renderer draws from the descriptor.

Run:  python Tools/Worldgen/viz_ms1b_ab.py
Emits: Docs/provenance_ms1b_ab_contact_sheet.png
"""
from __future__ import annotations
from pathlib import Path
from PIL import Image, ImageDraw

import macro_authority as MA
from macro_authority import unpack_surface

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "Docs"
PAGE_DIR = ROOT / "Data" / "Worldgen" / "MacroAuthority"
SCRATCH = Path(r"C:\Users\D-Day\AppData\Local\Temp\claude\C--Users-D-Day-ProvenanceEsoterica\64608940-1826-4869-b6c1-4bf5c20595bc\scratchpad")

# ---- mirror of Ms1::LithologyAlbedo / SubstrateAlbedo (base albedo, 0..1) --------------- #
LITH = {0:(0.62,0.58,0.55),1:(0.24,0.22,0.22),2:(0.78,0.63,0.41),3:(0.42,0.44,0.45),
        4:(0.80,0.79,0.71),5:(0.74,0.70,0.72),6:(0.52,0.48,0.54),7:(0.55,0.52,0.50)}
def _mix(a,b,t): return tuple(a[i]+(b[i]-a[i])*max(0,min(1,t)) for i in range(3))
def base_albedo(ss):
    la=LITH[ss_lith(ss)]
    s=ss_sub(ss)
    if s==0: c=la
    elif s==1: c=_mix(la,(0.46,0.38,0.30),0.30+0.30*ss.weathering)
    elif s==2: c=_mix(la,(0.52,0.43,0.31),0.35+0.45*ss.soil_depth)
    elif s==3: c=(0.50,0.42,0.33)
    elif s==4: c=_mix(la,(0.50,0.48,0.46),0.45)
    elif s==5: c=(0.66,0.58,0.43)
    elif s==6: c=(0.58,0.54,0.43)
    elif s==7: c=(0.60,0.55,0.45)
    elif s==8: c=(0.34,0.30,0.22)
    elif s==9: c=(0.31,0.33,0.33)
    elif s==10: c=(0.14,0.13,0.13)
    elif s==11: c=(0.33,0.21,0.17)
    elif s==12: c=(0.40,0.31,0.25)
    elif s==13: c=(0.29,0.25,0.19)
    else: c=la
    # regolith exposure pulls back toward host rock; wet darken (mirror of ResolveAppearance)
    fam=ss_fam(ss)
    if fam==1 or s==1: c=_mix(c,la,0.35*ss.exposure)
    wd=1-0.34*ss.wetness
    c=tuple(v*wd for v in c)
    return tuple(int(255*max(0,min(1,v))) for v in c)

def ss_sub(ss):  # substrate index via name
    return MA._SUBSTRATE_IDX[ss.substrate_class]
def ss_lith(ss): return MA._LITHOLOGY_IDX[ss.lithology_class]
def ss_fam(ss):  return MA._FAMILY_IDX[ss.dominant_surface_family]

def load_ppm(path):
    return Image.open(path).convert("RGB")

def label(img, text, h=22):
    out=Image.new("RGB",(img.width,img.height+h),(20,20,24))
    out.paste(img,(0,h)); d=ImageDraw.Draw(out); d.text((6,4),text,fill=(235,235,235)); return out

def far_material_map(scale=5):
    # assemble +/-160 km descriptor grid and paint base albedo (flat, top-down: material only)
    cells={}; sn=None; minri=minrj=9<<20; maxri=maxrj=-(9<<20)
    for f in PAGE_DIR.glob("page_*.mcp"):
        kv={}; codes=[]
        for line in f.read_text(encoding="utf-8").splitlines():
            if line.startswith("#") or "=" not in line: continue
            k,v=line.split("=",1)
            if k=="surface_grid_row_major": codes=[int(t,16) for t in v.split()]
            else: kv[k]=v
        if "region_cell" not in kv or not codes: continue
        ri,rj=(int(t) for t in kv["region_cell"].split(",")); sn=int(kv["surface_grid_n"])
        cells[(ri,rj)]=codes; minri,maxri=min(minri,ri),max(maxri,ri); minrj,maxrj=min(minrj,rj),max(maxrj,rj)
    bw=sn-1; W=(maxri-minri+1)*bw+1; H=(maxrj-minrj+1)*bw+1
    img=Image.new("RGB",(W,H),(30,30,34)); px=img.load()
    for (ri,rj),codes in cells.items():
        ox=(ri-minri)*bw; oy=(maxrj-rj)*bw
        for j in range(sn):
            for i in range(sn):
                gx,gy=ox+i,oy+(sn-1-j)
                if 0<=gx<W and 0<=gy<H: px[gx,gy]=base_albedo(unpack_surface(codes[j*sn+i]))
    return img.resize((W*scale,H*scale),Image.NEAREST)

def main():
    pairs=[("00","looking N"),("06","looking S")]
    rows=[]
    for tag,desc in pairs:
        off=load_ppm(DOCS/f"provenance_mv1rot_{tag}_mv2boff_cull.ppm").resize((560,337))
        on =load_ppm(SCRATCH/"ab_on"/f"provenance_mv1rot_{tag}_mv2boff_cull.ppm").resize((560,337))
        a=label(off,f"OFF  diagnostic palette (biome CapColor: alpine WHITE) — {desc}")
        b=label(on ,f"ON   MS1.B shared resolver: material-derived — {desc}")
        gap=Image.new("RGB",(12,a.height),(20,20,24))
        row=Image.new("RGB",(a.width+12+b.width,a.height),(20,20,24))
        row.paste(a,(0,0)); row.paste(gap,(a.width,0)); row.paste(b,(a.width+12,0)); rows.append(row)
    far=label(far_material_map(),
              "FAR origin world +/-160 km — MS1.B material appearance from the shipped page descriptor "
              "(volcanic dark / rock / regolith / sediment as MATERIAL, top-down flat; not an elevation ramp)")
    W=max(r.width for r in rows+[far]); pad=10
    H=sum(r.height for r in rows)+far.height+pad*(len(rows)+2)
    sheet=Image.new("RGB",(W,H),(16,16,20)); y=pad
    title=label(Image.new("RGB",(W,4),(16,16,20)),
                "MS1.B — one shared SurfaceState->appearance resolver retires the near/mid/far diagnostic palettes",26)
    sheet.paste(title,(0,0)); y=title.height+pad
    for r in rows: sheet.paste(r,(0,y)); y+=r.height+pad
    sheet.paste(far,(0,y))
    out=DOCS/"provenance_ms1b_ab_contact_sheet.png"; sheet.save(out); print("wrote",out)

if __name__=="__main__":
    main()
