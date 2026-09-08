"""Blender background export of static, transform-baked trial copies.

Never saves or modifies the artist's .blend/.glb inputs. Run with Blender's
--background --factory-startup --python; outputs are derived, not authority.
"""
import bpy
import hashlib
import json
from pathlib import Path
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / 'output' / 'integration' / 'nature_trial_v001'
SPECS = [
    ('healing_bush', 'berry_bushes/v001/healing_red_bush_ripe.glb', None),
    ('stamina_bush', 'berry_bushes/v001/stamina_blue_bush_ripe.glb', None),
    *[(name.lower(), 'mushrooms/v001/raised_mushrooms.glb', name + '_Group')
      for name in ('Chestnut', 'Bell', 'Ivory', 'Ochre')],
    ('cyan_pair', 'magical_mushrooms/v001/cyan_pair.glb', None),
    ('violet_lantern', 'magical_mushrooms/v001/violet_lantern.glb', None),
]
if DEST.exists():
    raise RuntimeError('Trial export directory exists; review before regenerating: ' + str(DEST))
DEST.mkdir(parents=True)
report = []
for asset_id, rel, group in SPECS:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    source = ROOT / 'output' / 'models' / rel
    bpy.ops.import_scene.gltf(filepath=str(source))
    if group:
        root = bpy.data.objects[group]
        root.location = (0, 0, 0)
        chosen = [o for o in root.children_recursive if o.type == 'MESH']
    else:
        chosen = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    bpy.context.view_layer.update()
    bounds = [o.matrix_world @ Vector(corner) for o in chosen for corner in o.bound_box]
    before = sum(len(o.data.polygons) for o in chosen)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in chosen:
        world = obj.matrix_world.copy()
        obj.parent = None
        obj.matrix_world = world
        obj.select_set(True)
    bpy.context.view_layer.objects.active = chosen[0]
    # Engine importer rotates vertex coordinates but not all node transforms.
    # Bake world transforms in this derivative to retain meters, fruit positions
    # and group arrangement without changing engine import code.
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    assert before == sum(len(o.data.polygons) for o in chosen)
    target = DEST / (asset_id + '.glb')
    bpy.ops.export_scene.gltf(filepath=str(target), export_format='GLB',
        use_selection=True, export_extras=True, export_lights=False,
        export_cameras=False, export_animations=False, export_yup=True)
    low = [min(p[k] for p in bounds) for k in range(3)]
    high = [max(p[k] for p in bounds) for k in range(3)]
    report.append(dict(id=asset_id, source=str(source),
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        export=str(target), meshes=len(chosen), bounds_z_up=[low, high],
        notes='Static visual copy; source sockets/gameplay metadata remain authoritative in original.'))
(DEST / 'export_report.json').write_text(json.dumps(report, indent=2))
print('NATURE_TRIAL_EXPORT_COMPLETE ' + str(DEST))
