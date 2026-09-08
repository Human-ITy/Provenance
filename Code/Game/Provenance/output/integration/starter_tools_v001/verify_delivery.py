import json,hashlib,xml.etree.ElementTree as ET
from pathlib import Path
from PIL import Image
P=Path(__file__).resolve().parent;ROOT=P.parents[2];REPO=ROOT.parents[2];art=ROOT/'output/models/tools/starter_stone/v001'
plan=json.loads((P/'native_plan.json').read_text(encoding='utf-8-sig'));dest=REPO/'Data/Provenance/Tools/StarterStone/v001'
for b in plan['binaries']:assert hashlib.sha256((dest/b['name']).read_bytes()).hexdigest()==b['sha256']
for f in plan['files']:assert Path(f['path']).read_text()==f['text']
def readmap(p):
    s=p.read_text();return ET.fromstring(s[s.index('<Entities>'):s.index('</Entities>')+11])
old=readmap(P/'sandbox_before.map.txt');new=readmap(REPO/'Data/Provenance/ProvenanceSandbox.map');names=['ToolPickup_StoneShovel','ToolPickup_StoneAxe','ToolPickup_StonePickaxe']
fmt=lambda e:ET.tostring(e,encoding='unicode').strip()
assert [fmt(e) for e in old]==[fmt(e) for e in new if e.get('Name') not in names]
for name in names:assert sum(e.get('Name')==name for e in new)==1
assert json.loads((art/'validation.json').read_text())['pass']
checks=[]
for entry in json.loads((art/'manifest.json').read_text())['assets']:
    for image in ['review.png','inventory_icon.png']:
        im=Image.open(art/entry['id']/image);assert im.mode=='RGBA' and im.getchannel('A').getextrema()==(0,255)
    if 'head_joint' in entry['anchors_z_up']:
        joint=entry['anchors_z_up']['head_joint'];cross=entry['anchors_z_up']['lashing_cross_front']
        assert abs(joint[0]-cross[0])<1e-9 and abs(joint[2]-cross[2])<1e-9
        checks.append({'id':entry['id'],'head_joint':joint,'cross_projection_error_m':0})
resources=json.loads((P/'resource_results.json').read_text(encoding='utf-8-sig'));assert len(resources)==56 and all(r['pass'] for r in resources)
installed=REPO/'Build/x64_Debug/Esoterica.Game.Runtime.dll';staged=P/'staged/Esoterica.Game.Runtime.dll'
newest=max((ROOT/f).stat().st_mtime for f in ['Geometry/ToolPickup.h','Systems/GraniteLabToolPickup.h','Systems/GraniteOutcropLab.cpp'])
target=installed if installed.stat().st_mtime>=newest else staged
assert target.is_file() and target.stat().st_mtime>=newest
binary=target.read_bytes();assert all(n.encode() in binary for n in names) and b'Tools: %d/4' in binary
result={'pass':True,'native_binary_hashes_verified':len(plan['binaries']),'native_descriptors_verified':len(plan['files']),'existing_entities_preserved':len(old),'added_pickups':3,'native_resource_compiles':56,'head_alignment':checks,'runtime_verified':str(target),'corrected_runtime_installed':target==installed,'runtime_sha256':hashlib.sha256(binary).hexdigest(),'interactive_editor_pickup_test':'not performed'}
(P/'delivery_validation.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
