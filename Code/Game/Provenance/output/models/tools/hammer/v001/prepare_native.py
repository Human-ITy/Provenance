from pathlib import Path
import json,hashlib
P=Path(__file__).resolve().parent;ROOT=P.parents[4];REPO=ROOT.parents[2]
derived=ROOT/'output/integration/hammer_v001';derived.mkdir(parents=True,exist_ok=True)
model=P/'guild_hammer_lod0.glb';m=json.loads((P/'manifest.json').read_text())
report=[{'id':'guild_hammer','source':str(model),'export':str(model),'source_sha256':hashlib.sha256(model.read_bytes()).hexdigest(),'bounds_z_up':m['bounds_z_up']}]
(derived/'export_report.json').write_text(json.dumps(report,indent=2))
s=(ROOT/'Verification/Scripts/PackageNatureTrial.cjs').read_text()
s=s.replace("output/integration/nature_trial_v001","output/integration/hammer_v001").replace('Data/Provenance/NatureTrial/v001','Data/Provenance/Tools/Hammer/v001').replace('data://provenance/naturetrial/v001/','data://provenance/tools/hammer/v001/')
(ROOT/'Verification/Scripts/PackageHammer.cjs').write_text(s)
print('Native descriptors ready to generate.')
