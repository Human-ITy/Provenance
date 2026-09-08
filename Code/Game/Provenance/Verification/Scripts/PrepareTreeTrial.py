"""Export a bounded v002 cohort from the grounded placement scene, not masters.
Blender --background --python-exit-code 1 --python this_file.py.
"""
import bpy
import json
import hashlib
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'output/placement/world_trees_v002/WorldTreePlacement.blend'
DEST = ROOT / 'output/integration/tree_trial_v002'
if DEST.exists():
    raise RuntimeError('Preserve existing derivative; use a new version to regenerate')
bpy.ops.wm.open_mainfile(filepath=str(SOURCE))
roots = [o for o in bpy.context.scene.objects if o.get('instance_id')]
# A small overstory tree plus regeneration in each of the seven groves.
# Retain saved IDs, positions and copied root conformity from the reviewed scene.
chosen = []
for grove in sorted({o['instance_id'].split('.')[1] for o in roots}):
    group = [o for o in roots if o['instance_id'].split('.')[1] == grove]
    for tier in ('overstory', 'regeneration'):
        eligible = [o for o in group if '.'+tier+'.' in o['instance_id']]
        def height(o):
            points = [m.matrix_world @ Vector(v) for m in o.children_recursive if m.type == 'MESH' for v in m.bound_box]
            return max(p.z for p in points)-min(p.z for p in points)
        # Young rather than tiny seedlings for the visible integration cohort.
        if tier == 'regeneration':
            eligible = [o for o in eligible if height(o)>1.0]
        chosen.append(min(eligible, key=height))
assert len(chosen)==14
DEST.mkdir(parents=True)
report=[]
for root in chosen:
    ident=root['instance_id']; asset_id=ident.replace('.', '_')
    meshes=[o for o in root.children_recursive if o.type=='MESH']
    bpy.ops.object.select_all(action='DESELECT')
    copies=[]
    for original in meshes:
        copy=original.copy();copy.data=original.data.copy()
        bpy.context.scene.collection.objects.link(copy)
        world=original.matrix_world.copy();copy.parent=None;copy.matrix_world=world
        copy.hide_set(False);copy.hide_viewport=False;copy.select_set(True);copies.append(copy)
    bpy.context.view_layer.objects.active=copies[0]
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    points=[o.matrix_world@Vector(v) for o in copies for v in o.bound_box]
    target=DEST/(asset_id+'.glb')
    bpy.ops.export_scene.gltf(filepath=str(target),export_format='GLB',use_selection=True,
        export_yup=True,export_extras=True,export_lights=False,export_cameras=False,export_animations=False)
    report.append(dict(id=asset_id,instance_id=ident,source=str(SOURCE),
        source_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),export=str(target),
        bounds_z_up=[[min(p[k] for p in points) for k in range(3)],[max(p[k] for p in points) for k in range(3)]],
        placement=list(root.location),notes='World-baked grounded LOD1 visual. Map transform identity. No chopping/collision.'))
    for o in copies:bpy.data.objects.remove(o,do_unlink=True)
(DEST/'export_report.json').write_text(json.dumps(report,indent=2))
print('TREE_TRIAL_EXPORT_COMPLETE',len(report))
