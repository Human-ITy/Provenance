"""Install only the reviewed hammer resources and its one sandbox entity."""
import json,subprocess,hashlib
from pathlib import Path
P=Path(__file__).resolve().parent;ROOT=P.parents[2];REPO=ROOT.parents[2]
plan=json.loads((P/'native_plan.json').read_text(encoding='utf-8-sig'))
allowed=(REPO/'Data/Provenance/Tools/Hammer/v001').resolve()
for f in plan['files']:
    path=Path(f['path']).resolve();assert path.parent==allowed
    if path.exists() and path.read_text()!=f['text']:raise RuntimeError('Conflicting existing hammer descriptor: '+str(path))
subprocess.run(['C:/Users/D-Day/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe',str(ROOT/'Verification/Scripts/PackageHammer.cjs'),'--install-binaries'],check=True)
for f in plan['files']:Path(f['path']).write_text(f['text'],encoding='utf-8')
mapfile=REPO/'Data/Provenance/ProvenanceSandbox.map';text=mapfile.read_text(encoding='utf-8')
placement=json.loads((ROOT/'output/models/tools/hammer/v001/placement.json').read_text());x,y,z=placement['world_position'];rx,ry,rz=placement['rotation_degrees']
entity=f'''    <Entity Name="ToolPickup_GuildHammer">
      <Components>
        <Type TypeID="EE::Render::StaticMeshComponent">
          <Property Path="m_name" Value="Guild hammer pickup" />
          <Property Path="m_transform" Value="{rx},{ry},{rz},{x},{y},{z},1" />
          <Property Path="m_viewLayers" Value="ShadowMap|ForwardShading" />
          <Property Path="m_mesh" Value="data://provenance/tools/hammer/v001/guild_hammer.mesh" />
        </Type>
      </Components>
      <ReferencedResources>
        <Resource ID="data://provenance/tools/hammer/v001/guild_hammer.mesh" />
      </ReferencedResources>
    </Entity>
'''
if 'ToolPickup_GuildHammer' not in text:
    assert text.count('  </Entities>')==1
    (P/'sandbox_before.map.txt').write_text(text,encoding='utf-8')
    updated=text.replace('  </Entities>',entity+'  </Entities>')
    mapfile.write_text(updated,encoding='utf-8')
(P/'resources.json').write_text(json.dumps({'resources':plan['resources']+['data://provenance/provenancesandbox.map'],'placement':placement},indent=2))
print('Installed hammer resource descriptors and one pickup entity, preserving all existing map entities.')
