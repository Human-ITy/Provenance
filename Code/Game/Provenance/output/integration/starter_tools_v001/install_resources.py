"""Install reviewed starter-tool resources and three separate sandbox pickups."""
import json,subprocess
from pathlib import Path
P=Path(__file__).resolve().parent;ROOT=P.parents[2];REPO=ROOT.parents[2]
plan=json.loads((P/'native_plan.json').read_text(encoding='utf-8-sig'));allowed=(REPO/'Data/Provenance/Tools/StarterStone/v001').resolve()
for f in plan['files']:
    path=Path(f['path']).resolve();assert path.parent==allowed
    if path.exists() and path.read_text()!=f['text']:raise RuntimeError('Conflicting descriptor: '+str(path))
subprocess.run(['C:/Users/D-Day/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe',str(ROOT/'Verification/Scripts/PackageStarterTools.cjs'),'--install-binaries'],check=True)
for f in plan['files']:Path(f['path']).write_text(f['text'],encoding='utf-8')
mapfile=REPO/'Data/Provenance/ProvenanceSandbox.map';text=mapfile.read_text(encoding='utf-8');original=text
placement=json.loads((P/'placement.json').read_text());names={'stone_shovel':'StoneShovel','stone_axe':'StoneAxe','stone_pickaxe':'StonePickaxe'}
for p in placement:
    name='ToolPickup_'+names[p['id']];x,y,z=p['world_position'];resource='data://provenance/tools/starterstone/v001/'+p['id']+'.mesh'
    if 'Name="'+name+'"' in text:continue
    entity=f'''    <Entity Name="{name}">
      <Components>
        <Type TypeID="EE::Render::StaticMeshComponent">
          <Property Path="m_name" Value="{p['id'].replace('_',' ')} pickup" />
          <Property Path="m_transform" Value="0,0,0,{x},{y},{z},1" />
          <Property Path="m_viewLayers" Value="ShadowMap|ForwardShading" />
          <Property Path="m_mesh" Value="{resource}" />
        </Type>
      </Components>
      <ReferencedResources>
        <Resource ID="{resource}" />
      </ReferencedResources>
    </Entity>
'''
    assert text.count('  </Entities>')==1;text=text.replace('  </Entities>',entity+'  </Entities>')
if text!=original:
    (P/'sandbox_before.map.txt').write_text(original,encoding='utf-8')
    assert mapfile.read_text(encoding='utf-8')==original,'Map changed during installation; retry using latest map.'
    mapfile.write_text(text,encoding='utf-8')
(P/'resources.json').write_text(json.dumps({'resources':plan['resources']+['data://provenance/provenancesandbox.map'],'placement':placement},indent=2))
print('Installed three starter-tool pickups, preserving existing sandbox entities.')
