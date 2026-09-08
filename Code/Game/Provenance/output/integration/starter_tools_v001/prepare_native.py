from pathlib import Path
import json,hashlib
P=Path(__file__).resolve().parent;ROOT=P.parents[2]
art=ROOT/'output/models/tools/starter_stone/v001';data=json.loads((art/'manifest.json').read_text())
report=[]
for entry in data['assets']:
    model=art/entry['id']/entry['visual_lods'][0]
    report.append({'id':entry['id'],'source':str(model),'export':str(model),'source_sha256':hashlib.sha256(model.read_bytes()).hexdigest(),'bounds_z_up':entry['bounds_z_up']})
(P/'export_report.json').write_text(json.dumps(report,indent=2))
s=(ROOT/'Verification/Scripts/PackageHammer.cjs').read_text().replace('output/integration/hammer_v001','output/integration/starter_tools_v001').replace('Data/Provenance/Tools/Hammer/v001','Data/Provenance/Tools/StarterStone/v001').replace('data://provenance/tools/hammer/v001/','data://provenance/tools/starterstone/v001/')
(ROOT/'Verification/Scripts/PackageStarterTools.cjs').write_text(s)
supports=json.loads((P/'placement_support.json').read_text());placement=[]
for e,support in zip(data['assets'],supports):
    assert e['id']==support['id'];pos=list(support['support_world']);pos[2]+=.003-e['bounds_z_up'][0][2]
    placement.append({**support,'world_position':pos,'rotation_degrees':[0,0,0],'scale':1})
(P/'placement.json').write_text(json.dumps(placement,indent=2))
print('Prepared three native tool exports and supported display positions.')
