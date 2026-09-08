"""Reopen saved blend and independently verify the placement handoff."""
import bpy,json,math,hashlib
from pathlib import Path
OUT=Path(__file__).resolve().parent
manifest=json.loads((OUT/'world_tree_placements.json').read_text())
roots={o.get('instance_id'):o for o in bpy.data.objects if o.get('instance_id')}
assert len(roots)==len(manifest['instances'])
triangles=0
for p in manifest['instances']:
    ob=roots[p['instance_id']]
    assert max(abs(ob.location[k]-p['position_world_m'][k]) for k in range(3))<1e-5
    assert max(abs(ob.scale[k]-p['uniform_scale']) for k in range(3))<1e-6
    assert abs(ob.rotation_euler.x)<1e-7 and abs(ob.rotation_euler.y)<1e-7
    assert abs(ob.rotation_euler.z-p['yaw_radians'])<1e-6
    for mesh in ob.children_recursive:
        if mesh.type!='MESH':continue
        assert not mesh.hide_render
        assert all(math.isfinite(v) for vertex in mesh.data.vertices for v in vertex.co)
        triangles+=sum(len(poly.vertices)-2 for poly in mesh.data.polygons)
assert triangles==manifest['budgets']['all_trees_triangles_by_lod']['1']
assert all(img.packed_file or img.source!='FILE' for img in bpy.data.images)
source_root=OUT/'../../models/trees/v002'
paths={Path(__file__),OUT/'build_world.py',OUT/'export_terrain.cpp',OUT/'layout_recipe.json',source_root/'manifest.json'}
for p in manifest['instances']:
    paths.add(source_root/p['collision_file'])
    for lod in p['visual_lods']:paths.add(source_root/lod['file'])
paths.update((OUT/'../../../Geometry').glob('PlayableLandscape.h'))
paths.update((OUT/'../../../Geometry').glob('GraniteOutcrop*.h'))
paths.add(OUT/'../../../Geometry/GraniteContactCast.h')
paths.add(OUT/'../../imagegen/leaf-litter/woodland_leaf_litter_v002_seamless_color.png')
source_hashes={str(p.resolve()):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(paths,key=str)}
(OUT/'source_hashes.json').write_text(json.dumps(source_hashes,indent=2))
report=json.loads((OUT/'validation.json').read_text())
old=json.loads((OUT.parent/'world_trees_v001/world_tree_placements.json').read_text())
before={p['instance_id']:p for p in old['instances']}
for p in manifest['instances']:
    q=before[p['instance_id']]
    assert all(p[k]==q[k] for k in ['position_world_m','target_height_m','yaw_radians','species','stage'])
report['original_positions_heights_yaws_ids_preserved']=True
report.update(saved_blend_reopened=True,saved_instance_transforms_match=True,
              packed_images=True,actual_lod1_tree_triangles=triangles,finite_mesh_coordinates=True,
              source_files_hashed=len(source_hashes))
(OUT/'validation.json').write_text(json.dumps(report,indent=2))
print('SAVED_SCENE_VALIDATION_PASS',len(roots),triangles,flush=True)
