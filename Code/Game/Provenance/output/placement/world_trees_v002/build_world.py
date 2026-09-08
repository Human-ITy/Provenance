"""Offline tree authoring on exported C++ terrain; no runtime map writes.
Run with Blender --background --python build_world.py.
Edit layout_recipe.json to change grove locations/counts; IDs use grove/slot.
"""
import bpy
import numpy as np
import json, math, random, hashlib, struct, subprocess
from pathlib import Path
from mathutils import Vector

OUT = Path(__file__).resolve().parent
ASSETS = OUT.parents[1] / 'models/trees/v002'
RECIPE = json.loads((OUT / 'layout_recipe.json').read_text())
CATALOG = json.loads((ASSETS / 'manifest.json').read_text())
ASSET = {a['asset_id']: a for a in CATALOG['assets']}

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def seeded(key): return int.from_bytes(hashlib.sha256(f"{RECIPE['seed']}:{key}".encode()).digest()[:8], 'little')
with (OUT / 'terrain.bin').open('rb') as f:
    NX, NY, NF = struct.unpack('<III', f.read(12))
    XS = np.fromfile(f, '<f8', NX); YS = np.fromfile(f, '<f8', NY)
    VALUES = np.fromfile(f, '<f8', NX * NY * 3).reshape(NY, NX, 3)
    FACES = np.fromfile(f, '<u4', NF * 3).reshape(-1, 3)
    CREEK = np.fromfile(f, '<f8', NY * 3).reshape(-1, 3)

def ground(x, y):
    ix = np.clip(np.searchsorted(XS, x, side='right') - 1, 0, NX-2)
    iy = np.clip(np.searchsorted(YS, y, side='right') - 1, 0, NY-2)
    u = (x-XS[ix])/(XS[ix+1]-XS[ix]); v = (y-YS[iy])/(YS[iy+1]-YS[iy])
    a, b = VALUES[iy,ix,0], VALUES[iy,ix+1,0]
    c, d = VALUES[iy+1,ix+1,0], VALUES[iy+1,ix,0]
    return np.where(u >= v, a+(b-a)*u+(c-b)*v, a+(c-d)*u+(d-a)*v)

def segment_distance(x, y, a, b):
    dx,dy=b[0]-a[0],b[1]-a[1]
    t=max(0,min(1,((x-a[0])*dx+(y-a[1])*dy)/(dx*dx+dy*dy)))
    return math.hypot(x-a[0]-t*dx,y-a[1]-t*dy)

def eligible(x, y, root, crown, tall, placements):
    footprint=max(root, crown*.8)
    if max(abs(x),abs(y))+footprint > 48.5: return False
    if math.hypot(max(-5.4-x,x-5.64,0),max(31-y,y-41.56,0)) < root+3: return False
    for clearing in RECIPE['clearings']:
        cx,cy=clearing['center'];rx,ry=clearing['radius']
        if ((x-cx)/(rx+crown*.6))**2+((y-cy)/(ry+crown*.6))**2 < 1: return False
    for path in RECIPE['walk_corridors']:
        if segment_distance(x,y,path['a'],path['b']) < path['half_width_m']+max(root*.5,.2): return False
    angles=np.arange(24)*math.tau/24
    px=x+root*np.cos(angles);py=y+root*np.sin(angles)
    cx=np.interp(py,YS,CREEK[:,0]);slope=np.interp(py,YS,CREEK[:,1]);width=np.interp(py,YS,CREEK[:,2])
    if np.any(np.abs(px-cx)/np.sqrt(1+slope*slope) < width*.62+1.6): return False
    # Gentle, supported sites only; copied roots are conformed later.
    heights=ground(px,py)
    if np.max(np.abs(heights-ground(x,y))) > max(.08,root*.13): return False
    for p in placements:
        distance=math.hypot(x-p['position_world_m'][0],y-p['position_world_m'][1])
        if tall and p['target_height_m']>=7:
            if distance < .87*(crown+p['crown_radius_m'])+1: return False
        elif distance < max(.7,(root+p['root_radius_m'])*.55): return False
    return True

def make_layout():
    result=[]
    # Canopy first, then smaller trees: regeneration must fit the final overstory.
    for tier in ('overstory','regeneration'):
        for grove in RECIPE['groves']:
            for slot in range(grove[tier]):
                tree_id=f"tree.{grove['id']}.{tier}.{slot:03d}"
                rng=random.Random(seeded(tree_id))
                species=grove['species']
                if 'mixed' in grove['id'] and rng.random()<.3: species='silver_birch'
                if tier=='overstory':
                    stages=['adolescent','adolescent','grown','adolescent','grown']
                    stage='mature' if slot==0 else stages[(slot-1)%len(stages)]
                    if grove['id']=='northwest_spruce' and slot==0: stage='ancient'
                else: stage=rng.choices(['seedling','sapling','young'],weights=[2,4,4])[0]
                asset=ASSET[f'{species}_{stage}']
                lo,hi=asset['recommended_common_height_range_m']
                height=rng.uniform(lo,hi);scale=height/asset['nominal_height_m']
                root=asset['root_radius_m']*scale;crown=asset['crown_radius_m']*scale
                for attempt in range(1800):
                    angle=rng.uniform(0,math.tau)
                    radius=math.sqrt(rng.random())
                    # Younger trees favor the brighter outside of a grove.
                    if tier=='regeneration': radius=.55+.65*radius
                    x=grove['center'][0]+math.cos(angle)*radius*grove['radius'][0]
                    y=grove['center'][1]+math.sin(angle)*radius*grove['radius'][1]
                    if eligible(x,y,root,crown,height>=7,result): break
                else: raise RuntimeError(f'No supported site for {tree_id}; adjust recipe, never silently drop slots')
                yaw=rng.uniform(0,math.tau)
                result.append(dict(instance_id=tree_id, grove_id=grove['id'], asset_id=asset['asset_id'],
                    species=species,stage=stage,target_height_m=height,uniform_scale=scale,
                    position_world_m=[x,y,float(ground(x,y))-.025],yaw_radians=yaw,
                    root_radius_m=root,crown_radius_m=crown,source_seed=seeded(tree_id),
                    visual_lods=asset['lods'],collision_file=asset['collision_file'],
                    root_placement='gravity_up; 25mm collar burial; preview copies conformed, see root_conform records'))
    return result

placements=json.loads((OUT/'accepted_placements.json').read_text())
assert len(placements)<=RECIPE['limits']['max_instances']
totals={str(i):sum(ASSET[p['asset_id']]['lods'][i]['triangles'] for p in placements) for i in range(3)}
assert totals['1']<=RECIPE['limits']['max_all_lod1_triangles']
(OUT/'placement_probes.tsv').write_text(''.join(f"{p['instance_id']} {p['position_world_m'][0]:.15g} {p['position_world_m'][1]:.15g} {p['position_world_m'][2]:.15g} {p['root_radius_m']:.15g}\n" for p in placements))
check=subprocess.run([str(OUT/'export_terrain.exe'),str(OUT/'placement_probes.tsv')],capture_output=True,text=True,check=True)
print(check.stdout,flush=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.unit_settings.system='METRIC'
templates=bpy.data.collections.new('Source LOD1 templates - hidden');scene.collection.children.link(templates);templates.hide_render=True;templates.hide_viewport=True
masters={}
groves={}
for g in RECIPE['groves']:
    collection=bpy.data.collections.new(g['id']);scene.collection.children.link(collection);groves[g['id']]=collection
for aid in sorted({p['asset_id'] for p in placements}):
    before=set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(ASSETS/ASSET[aid]['lods'][1]['file']))
    objects=list(set(bpy.data.objects)-before)
    root=next(o for o in objects if o.get('asset_id')==aid)
    for o in objects:
        for collection in list(o.users_collection): collection.objects.unlink(o)
        templates.objects.link(o)
    masters[aid]=(root,objects)

for p in placements:
    original,objects=masters[p['asset_id']];copies={o:o.copy() for o in objects}
    for source,copy in copies.items():
        groves[p['grove_id']].objects.link(copy)
        copy.parent=copies.get(source.parent)
        copy.name=p['instance_id']+'.'+str(source.get('part','root'))
    root=copies[original];root.location=p['position_world_m'];root.scale=(p['uniform_scale'],)*3;root.rotation_euler.z=p['yaw_radians']
    root['instance_id']=p['instance_id'];root['placement_status']='AUTHORING ONLY - engine integration pending'
    bpy.context.view_layer.update()
    conform=[]
    for source,o in copies.items():
        if o.get('part')!='roots':continue
        o.data=o.data.copy()
        coords=np.array([v.co[:] for v in o.data.vertices],dtype=float)
        matrix=np.array(o.matrix_world);world=coords@matrix[:3,:3].T+matrix[:3,3]
        x,y,_=p['position_world_m'];dist=np.hypot(world[:,0]-x,world[:,1]-y)
        allowance=np.where(dist>min(.30*p['uniform_scale'],p['root_radius_m']*.22),.035,.07)
        ceiling=ground(world[:,0],world[:,1])+allowance
        delta=np.maximum(0,world[:,2]-ceiling);world[:,2]-=delta
        inverse=np.linalg.inv(matrix);coords=world@inverse[:3,:3].T+inverse[:3,3]
        o.data.vertices.foreach_set('co',coords.astype(np.float32).ravel());o.data.update()
        # Imported custom normals are no longer valid after deformation.
        if o.data.has_custom_normals:
            o.data.normals_split_custom_set([(0,0,0)]*len(o.data.loops))
        conform.append(dict(part=source.get('part'),vertices_adjusted=int(np.sum(delta>1e-8)),max_lowering_m=float(delta.max()),
                            policy='z=min(z, exact_ground+allowance); allowance 35mm outer / 70mm collar',topology_changed=False))
    p['root_conform']=conform

def mesh_object(name,vertices,faces):
    me=bpy.data.meshes.new(name);me.from_pydata(vertices,[],faces);me.update()
    ob=bpy.data.objects.new(name,me);scene.collection.objects.link(ob)
    for poly in me.polygons:poly.use_smooth=True
    return ob

gx,gy=np.meshgrid(XS,YS)
vertices=np.column_stack((gx.ravel(),gy.ravel(),VALUES[:,:,0].ravel()))
terrain=mesh_object('Actual playable landscape - original C++ triangulation',vertices,FACES)

def attribute(ob,name,data):
    attr=ob.data.attributes.new(name,'FLOAT','POINT');attr.data.foreach_set('value',np.asarray(data,dtype=np.float32).ravel())

def litter_values(x,y):
    value=np.zeros_like(x)
    for p in placements:
        if p['species']!='silver_birch' or p['target_height_m']<7:continue
        px,py,_=p['position_world_m'];r=p['crown_radius_m'];phase=(p['source_seed']%10000)*.001
        angle=np.arctan2(y-py,x-px)
        boundary=r*(1.08+.16*np.sin(angle*3+phase)+.10*np.cos(angle*5-phase))
        d=np.hypot((x-px)*.91,(y-py)*1.07)/np.maximum(.1,boundary)
        mask=np.clip((1.22-d)/.42,0,1);mask=mask*mask*(3-2*mask)
        mask*=.58+.25*np.sin(x*1.9+y*.6+phase)*np.sin(y*2.4-x*.4)
        value=1-(1-value)*(1-mask)
    return value

attribute(terrain,'GroundCover',VALUES[:,:,1]);attribute(terrain,'Mud',VALUES[:,:,2])
attribute(terrain,'BroadleafLitter',litter_values(gx,gy))

def ground_material():
    mat=bpy.data.materials.new('Authoring ground: coverage + birch litter (not runtime shader)');mat.use_nodes=True
    nt=mat.node_tree;n=nt.nodes;l=nt.links;bs=n['Principled BSDF'];bs.inputs['Roughness'].default_value=.94
    geom=n.new('ShaderNodeNewGeometry');noise=n.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=7;l.new(geom.outputs['Position'],noise.inputs['Vector'])
    grass=n.new('ShaderNodeValToRGB');grass.color_ramp.elements[0].color=(.10,.13,.036,1);grass.color_ramp.elements[1].color=(.26,.30,.095,1);l.new(noise.outputs['Fac'],grass.inputs[0])
    cover=n.new('ShaderNodeAttribute');cover.attribute_name='GroundCover'
    mix=n.new('ShaderNodeMixRGB');mix.inputs[1].default_value=(.24,.16,.087,1);l.new(cover.outputs['Fac'],mix.inputs[0]);l.new(grass.outputs[0],mix.inputs[2])
    mud=n.new('ShaderNodeAttribute');mud.attribute_name='Mud';mudmix=n.new('ShaderNodeMixRGB');l.new(mud.outputs['Fac'],mudmix.inputs[0]);l.new(mix.outputs[0],mudmix.inputs[1]);mudmix.inputs[2].default_value=(.095,.065,.039,1)
    texture=n.new('ShaderNodeTexImage');texture.image=bpy.data.images.load(str(OUT.parents[1]/'imagegen/leaf-litter/woodland_leaf_litter_v002_seamless_color.png'));texture.extension='REPEAT';l.new(geom.outputs['Position'],texture.inputs['Vector'])
    mask=n.new('ShaderNodeAttribute');mask.attribute_name='BroadleafLitter';leaf=n.new('ShaderNodeMixRGB');l.new(mask.outputs['Fac'],leaf.inputs[0]);l.new(mudmix.outputs[0],leaf.inputs[1]);l.new(texture.outputs['Color'],leaf.inputs[2]);l.new(leaf.outputs[0],bs.inputs['Base Color'])
    bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.18;bump.inputs['Distance'].default_value=.02;l.new(noise.outputs['Fac'],bump.inputs['Height']);l.new(bump.outputs[0],bs.inputs['Normal'])
    return mat

soilmat=ground_material();terrain.data.materials.append(soilmat)
def triangles_file(path,name):
    with path.open('rb') as f:
        count=struct.unpack('<I',f.read(4))[0];verts=np.fromfile(f,'<f8',count*9).reshape(-1,3)
    return mesh_object(name,verts,np.arange(count*3).reshape(-1,3))
core=triangles_file(OUT/'core.bin','Protected granite lab ground - unchanged geometry');core.data.materials.append(soilmat)
attribute(core,'GroundCover',np.ones(len(core.data.vertices)));attribute(core,'Mud',np.zeros(len(core.data.vertices)));attribute(core,'BroadleafLitter',np.zeros(len(core.data.vertices)))
rock=triangles_file(OUT/'granite.bin','Existing authored granite - reference geometry')
mat=bpy.data.materials.new('Granite reference - authoring approximation');mat.use_nodes=True
nt=mat.node_tree;bs=nt.nodes['Principled BSDF'];bs.inputs['Roughness'].default_value=.85
noise=nt.nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=55
ramp=nt.nodes.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].color=(.10,.12,.12,1);ramp.color_ramp.elements[1].color=(.48,.49,.47,1)
nt.links.new(noise.outputs['Fac'],ramp.inputs[0]);nt.links.new(ramp.outputs[0],bs.inputs['Base Color']);rock.data.materials.append(mat)

litter_sources=[]
for p in placements:
    if p['target_height_m']<7:continue
    source=dict(source_id='litter.'+p['instance_id'],tree_instance_id=p['instance_id'],position_world_m=p['position_world_m'],
        canopy_radius_m=p['crown_radius_m'],seed=p['source_seed'],receiver='terrain surface; not square prop',
        status='Authored initial appearance footprint only; no simulated accumulation or material mass',
        lifecycle_handoff='Do not remove accumulated litter when tree is removed; persistence/decay requires engine integration')
    source['family']='broadleaf' if p['species']=='silver_birch' else 'needle_cone'
    source['texture']='imagegen/leaf-litter/woodland_leaf_litter_v002_seamless_color.png' if source['family']=='broadleaf' else None
    source['enabled_preview']=source['family']=='broadleaf'
    litter_sources.append(source)

manifest=dict(schema_version=1,status='OFFLINE AUTHORING LAYOUT; NOT LOADED BY C++ ENGINE',seed=RECIPE['seed'],
    coordinates=dict(units='metres',space='engine world / Blender Z-up',floor_xy=[[-50,-50],[50,50]],
                     outcrop_origin_world=[-1.4,35,.12],gltf_note='GLBs are Y-up; importer must convert once to engine Z-up before applying these world transforms'),
    recipe=RECIPE,terrain_sha256=sha(OUT/'terrain.bin'),asset_manifest_sha256=sha(ASSETS/'manifest.json'),
    source_root_relative='../../models/trees/v002',preview_lod=1,instances=placements,litter_sources=litter_sources,
    preview_limits='Original terrain/rock geometry; approximate ground/rock materials, no runtime grass, wind, excavation or collisions',
    budgets=dict(instances=len(placements),all_trees_triangles_by_lod=totals,game_fps='NOT MEASURED; native integration deferred'))
(OUT/'world_tree_placements.json').write_text(json.dumps(manifest,indent=2))
validation=dict(status='PASS',deterministic=True,unique_ids=len({p['instance_id'] for p in placements})==len(placements),
    exact_cpp_ground_check=check.stdout.strip(),instances=len(placements),all_lod_triangles=totals,
    source_files_exist=all((ASSETS/p['collision_file']).is_file() and all((ASSETS/l['file']).is_file() for l in p['visual_lods']) for p in placements),
    height_ranges_valid=all(ASSET[p['asset_id']]['allowed_height_range_m'][0]<=p['target_height_m']<=ASSET[p['asset_id']]['allowed_height_range_m'][1] for p in placements),
    runtime_validated=False,collision_registered=False)
assert validation['unique_ids'] and validation['source_files_exist'] and validation['height_ranges_valid']
(OUT/'validation.json').write_text(json.dumps(validation,indent=2))

world=bpy.data.worlds.new('Soft daylight');scene.world=world;world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.5,.62,.78,1);world.node_tree.nodes['Background'].inputs[1].default_value=.45
bpy.ops.object.light_add(type='SUN',location=(0,0,60));sun=bpy.context.object;sun.rotation_euler=(.46,-.5,-.7);sun.data.energy=2.3;sun.data.angle=.16
def camera(name,position,target,orthographic=None):
    bpy.ops.object.camera_add(location=position);ob=bpy.context.object;ob.name=name
    ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()
    if orthographic: ob.data.type='ORTHO';ob.data.ortho_scale=orthographic
    else: ob.data.lens=27
    ob.data.clip_end=500
    return ob
overview=camera('Overview - full playable floor',(112,-133,125),(0,0,7),159)
creekview=camera('Creek and birch margin',(-9,-4,float(ground(-9,-4))+1.75),(-32,10,8))
scene.camera=overview;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.resolution_x=1600;scene.render.resolution_y=1200;scene.render.resolution_percentage=100;scene.view_settings.view_transform='AgX'
# Packed asset textures and copied roots make this an independently editable scene.
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WorldTreePlacement.blend'))
for cam,filename in [(overview,'world_overview.png'),(creekview,'creek_birch_view.png')]:
    scene.camera=cam;scene.render.filepath=str(OUT/filename);bpy.ops.render.render(write_still=True)
print('WORLD_TREE_AUTHORING_COMPLETE',len(placements),totals,flush=True)
