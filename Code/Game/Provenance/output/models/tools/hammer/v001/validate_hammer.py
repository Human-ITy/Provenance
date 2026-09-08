"""Re-import both delivery models and check portable mesh/material data."""
import bpy,json,math
from pathlib import Path
P=Path(__file__).resolve().parent
manifest=json.loads((P/'manifest.json').read_text())
report=[]
for name in manifest['visual_lods']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(P/name))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    assert len(meshes)==4
    points=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
    assert all(math.isfinite(v) for p in points for v in p)
    bounds=[[min(p[k] for p in points) for k in range(3)],[max(p[k] for p in points) for k in range(3)]]
    assert max(abs(bounds[i][k]-manifest['bounds_z_up'][i][k]) for i in range(2) for k in range(3))<.002
    triangles=0
    for o in meshes:
        assert o.data.uv_layers.active
        assert len(o.data.materials)==1
        o.data.calc_loop_triangles();triangles+=len(o.data.loop_triangles)
        assert all(t.area>1e-14 for t in o.data.loop_triangles)
    images=[i for i in bpy.data.images if i.type=='IMAGE']
    assert len(images)==8 and all(tuple(i.size)==(1024,1024) for i in images)
    report.append({'file':name,'mesh_parts':len(meshes),'triangles':triangles,'bounds_z_up':bounds,'embedded_textures':len(images),'uvs':True,'finite_vertices':True,'nonzero_triangles':True})
assert report[1]['triangles']<report[0]['triangles']
(P/'validation.json').write_text(json.dumps({'pass':True,'reimported_lods':report},indent=2))
print('HAMMER_VALIDATION_PASS',[(r['file'],r['triangles']) for r in report])
