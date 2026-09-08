import bpy,json,math
from pathlib import Path
P=Path(__file__).resolve().parent;data=json.loads((P/'manifest.json').read_text());results=[]
for entry in data['assets']:
    rows=[]
    for name in entry['visual_lods']:
        bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(P/entry['id']/name))
        obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(obs)==len(entry['materials'])
        pts=[o.matrix_world@v.co for o in obs for v in o.data.vertices];assert all(math.isfinite(v) for p in pts for v in p)
        bounds=[[min(p[k] for p in pts) for k in range(3)],[max(p[k] for p in pts) for k in range(3)]]
        assert max(abs(bounds[i][k]-entry['bounds_z_up'][i][k]) for i in range(2) for k in range(3))<.004
        tris=0;zero=0
        for o in obs:
            assert o.data.uv_layers.active and len(o.data.materials)==1
            o.data.calc_loop_triangles();tris+=len(o.data.loop_triangles);zero+=sum(t.area<1e-14 for t in o.data.loop_triangles)
        assert zero==0,(name,zero)
        images=[i for i in bpy.data.images if i.type=='IMAGE'];assert len(images)==len(obs)*2 and all(tuple(i.size)==(1024,1024) for i in images)
        rows.append({'file':name,'triangles':tris,'materials':len(obs),'embedded_textures':len(images),'bounds_z_up':bounds,'nonzero_triangles':True,'uvs':True})
    assert rows[1]['triangles']<rows[0]['triangles'];results.append({'id':entry['id'],'lods':rows})
(P/'validation.json').write_text(json.dumps({'pass':True,'assets':results},indent=2));print('THREE_TOOLS_SIX_LODS_VALIDATED')
