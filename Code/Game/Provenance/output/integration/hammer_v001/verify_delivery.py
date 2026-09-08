import json,hashlib,xml.etree.ElementTree as ET
from pathlib import Path
from PIL import Image
P=Path(__file__).resolve().parent;ROOT=P.parents[2];REPO=ROOT.parents[2]
model=ROOT/'output/models/tools/hammer/v001'
plan=json.loads((P/'native_plan.json').read_text(encoding='utf-8-sig'))
dest=REPO/'Data/Provenance/Tools/Hammer/v001'
for b in plan['binaries']:
    assert hashlib.sha256((dest/b['name']).read_bytes()).hexdigest()==b['sha256']
for f in plan['files']:assert Path(f['path']).read_text()==f['text']
def read_map(path):
    text=path.read_text();text=text[text.index('<Entities>'):text.index('</Entities>')+len('</Entities>')]
    return ET.fromstring(text)
old=read_map(P/'sandbox_before.map.txt');new=read_map(REPO/'Data/Provenance/ProvenanceSandbox.map')
assert old is not None and new is not None
def entity(e):return ET.tostring(e,encoding='unicode').strip()
remaining=[e for e in new if e.get('Name')!='ToolPickup_GuildHammer']
assert [entity(e) for e in old]==[entity(e) for e in remaining]
pickup=[e for e in new if e.get('Name')=='ToolPickup_GuildHammer'];assert len(pickup)==1
position=json.loads((model/'placement.json').read_text())
transform=next(p.get('Value') for p in pickup[0].iter('Property') if p.get('Path')=='m_transform')
assert [float(x) for x in transform.split(',')]==position['rotation_degrees']+position['world_position']+[1]
dll=REPO/'Build/x64_Debug/Esoterica.Game.Runtime.dll';binary=dll.read_bytes()
assert b'E - Pick up guild hammer' in binary and b'Collected: Guild hammer' in binary
assert dll.stat().st_mtime>=max((ROOT/f).stat().st_mtime for f in ['Geometry/ToolPickup.h','Systems/GraniteLabToolPickup.h','Systems/GraniteOutcropLab.cpp'])
icons=[]
for name in ['hammer_review.png','hammer_inventory_icon.png']:
    im=Image.open(model/name);assert im.mode=='RGBA';assert im.getchannel('A').getextrema()==(0,255)
    icons.append({'file':name,'size':list(im.size),'genuine_alpha':True})
resources=json.loads((P/'resource_results.json').read_text(encoding='utf-8-sig'));assert len(resources)==18 and all(r['pass'] for r in resources)
result={'pass':True,'installed_binary_hashes_verified':len(plan['binaries']),'installed_descriptors_verified':len(plan['files']),'existing_map_entities_preserved':len(old),'hammer_entities':1,'transform_matches_placement':True,'built_dll_contains_pickup_prompts':True,'built_dll_newer_than_pickup_source':True,'native_compiles_passed':len(resources),'images':icons,'interactive_editor_pickup_test':'not performed'}
(P/'delivery_validation.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
# Index this asset alongside the rest of the placement handoff.
profiles_file=ROOT/'output/placement/asset_profiles.json';data=json.loads(profiles_file.read_text())
profile={'id':'guild_hammer','title':'Guild hammer pickup','directories':['models/tools/hammer/v001','integration/hammer_v001'],'preferred_files':['models/tools/hammer/v001/GuildHammer.blend','models/tools/hammer/v001/guild_hammer_lod0.glb','models/tools/hammer/v001/guild_hammer_lod1.glb','models/tools/hammer/v001/hammer_inventory_icon.png','models/tools/hammer/v001/manifest.json'],'status':'Modeled and installed in the Debug sandbox; E pickup implemented and compiled; interactive editor check pending.','habitat':'Workshop, workbench, salvage cache, ruin or deliberately placed selection display.','placement':'Metre scale: 0.662 m tall, 0.389 m head span. Pivot at pommel; grip anchor local (0,0,0.143). One upright sandbox item sits at world (1.03,35.40,0.514029303) on granite. For loose placement, align the whole head/grip footprint to support and retain visibility above grass.','variation':'Keep authored size unless deliberate design calls for resizing. Randomize yaw for loose props. LOD1 is supplied but automatic switching is not connected.','lifecycle':'Aim within 2 m and press E to collect once per Play session. Terrain obstruction and active text input block pickup. Model hides and collection prompt confirms ownership. New Play resets it; R terrain reset retains ownership.','limits':'Current single named item is a development-lab adapter, not persistent inventory/equipping. No rigid-body falling when support is excavated; decorative prop occlusion is not covered. Multi-item spawning requires unique instance records. Icon supplied for future UI.'}
data['profiles']=[p for p in data['profiles'] if p['id']!='guild_hammer']+[profile]
profiles_file.write_text(json.dumps(data,indent=2),encoding='utf-8')
