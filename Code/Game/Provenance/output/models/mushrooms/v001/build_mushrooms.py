"""Build original mushroom meshes in Blender 5.2; measurements in meters."""
import bpy, math, json, random
from pathlib import Path
import numpy as np
from mathutils import Vector

OUT = Path(__file__).resolve().parent
random.seed(47)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
assets = bpy.data.collections.new('Mushroom Assets')
scene.collection.children.link(assets)
stage = bpy.data.collections.new('Preview Stage - excluded from GLB')
scene.collection.children.link(stage)

def move_to(obj, collection):
    for c in list(obj.users_collection): c.objects.unlink(obj)
    collection.objects.link(obj)

def texture(name, color, kind, seed):
    n = 512
    u, v = np.meshgrid(np.arange(n)/n, np.arange(n)/n)
    rng = np.random.default_rng(seed)
    field = np.zeros((n,n))
    for k in range(28):
        fx = int(rng.integers(1, 52 if kind == 'stem' else 28))
        fy = int(rng.integers(1, 5 if kind == 'stem' else 30))
        if kind == 'cap':
            px=v*np.cos(u*math.tau);py=v*np.sin(u*math.tau)
            field += np.sin(2*np.pi*(fx*px + fy*py) + rng.uniform(0,6.28))/(1+k*.18)
        else:
            field += np.sin(2*np.pi*(fx*u + fy*v) + rng.uniform(0,6.28))/(1+k*.18)
    field /= 5
    if kind == 'stem':
        field += .3*np.sin(2*np.pi*(103*u+.2*np.sin(v*12)))
        tone = .95 + .14*field - .08*(1-v)**4
    elif kind == 'gills':
        tone = .96 + .09*field + .1*np.cos(u*2*np.pi*32)
    else:
        px=v*np.cos(u*math.tau);py=v*np.sin(u*math.tau)
        speck = np.maximum(0,np.sin(2*np.pi*(69*px+43*py))*np.sin(2*np.pi*(31*px-71*py))-.65)
        tone = .99 + .27*field - 1.1*speck
        tone += .09*np.maximum(0,(v-.84)/.16)
    rgb = np.clip(np.array(color)[None,None,:]*tone[:,:,None], 0, 1)
    rgba = np.concatenate((rgb, np.ones((n,n,1))),axis=2).astype(np.float32)
    img = bpy.data.images.new(name, width=n, height=n, alpha=False)
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = str(OUT/(name+'.png'))
    img.file_format = 'PNG'; img.save(); img.pack()
    mat = bpy.data.materials.new(name.replace('_Color',''))
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Roughness'].default_value = .76 if kind!='cap' else .65
    tex = mat.node_tree.nodes.new('ShaderNodeTexImage'); tex.image = img
    mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs['Base Color'])
    return mat

caps = [texture('Chestnut_Cap_Color',(.49,.265,.115),'cap',4),
        texture('Bell_Cap_Color',(.73,.535,.285),'cap',8),
        texture('Ivory_Cap_Color',(.83,.76,.58),'cap',12),
        texture('Ochre_Cap_Color',(.60,.425,.235),'cap',16)]
stemmat = texture('Stem_Color',(.82,.755,.58),'stem',22)
gillmat = texture('Gills_Color',(.67,.605,.46),'gills',24)

def mushroom(name, family, height, radius, lean, seed, parent, xy):
    rng = random.Random(seed)
    verts, faces, uvfaces, mats = [], [], [], []
    phase = rng.uniform(0,math.tau)
    def center(t):
        return (lean[0]*(t*t+.1*math.sin(t*math.pi)),lean[1]*t*t)
    capcenter = center(1)
    stem_radius = radius * [.235,.125,.30,.32][family]
    capdepth = radius * [.80,1.40,.30,.65][family]
    rimheight = height-capdepth
    def add_face(ids,uv,mi):
        faces.append(ids);uvfaces.append(uv);mats.append(mi)
    def surface(rings, seg, func, material, reverse=False):
        start = len(verts)
        for j in range(rings+1):
            for i in range(seg): verts.append(func(j/rings, math.tau*i/seg))
        for j in range(rings):
            for i in range(seg):
                ids=[start+j*seg+i,start+j*seg+(i+1)%seg,start+(j+1)*seg+(i+1)%seg,start+(j+1)*seg+i]
                uv=[(i/seg,j/rings),((i+1)/seg,j/rings),((i+1)/seg,(j+1)/rings),(i/seg,(j+1)/rings)]
                if reverse: ids.reverse();uv.reverse()
                add_face(ids,uv,material)
        return start
    def asym(a): return 1+.045*math.sin(3*a+phase)+.024*math.sin(7*a-phase)
    def rimwave(a): return radius*(.028 if family!=2 else .075)*math.sin(5*a+phase)
    def cap(t,a):
        r=radius*(.001+.999*t)*asym(a)
        if family==1: z=capdepth*(1-t**1.6)**1.4
        elif family==2: z=capdepth*(1-t*t)**.65 - radius*.035*math.exp(-t*t*35)
        else: z=capdepth*max(0,1-t*t)**.58
        return (capcenter[0]+r*math.cos(a),capcenter[1]+r*math.sin(a),rimheight+z+rimwave(a)*t*t)
    # Winding for outward normals on the upper cap.
    capstart=surface(16,64,cap,0,True)
    # Tiny center hole is closed with a fan rather than degenerate quads.
    ci=len(verts);verts.append((capcenter[0],capcenter[1],cap(0,0)[2]))
    for i in range(64): add_face([ci,capstart+i,capstart+(i+1)%64],[(.5,0),(i/64,0),((i+1)/64,0)],0)
    def underside(t,a):
        r=radius*(.001+.999*t)*asym(a)
        dip=radius*.11*(1-t)
        ribs=radius*.018*(.5+.5*math.cos(32*a))*math.sin(math.pi*t)**.5
        return (capcenter[0]+r*math.cos(a),capcenter[1]+r*math.sin(a),rimheight-dip-ribs+rimwave(a)*t*t)
    understart=surface(8,64,underside,2,False)
    ci=len(verts);verts.append((capcenter[0],capcenter[1],underside(0,0)[2]))
    for i in range(64): add_face([ci,understart+(i+1)%64,understart+i],[(.5,0),((i+1)/64,0),(i/64,0)],2)
    # Connect cap surface and gills at the lip.
    for i in range(64):
        nxt=(i+1)%64
        add_face([capstart+16*64+i,understart+8*64+i,understart+8*64+nxt,capstart+16*64+nxt],[(i/64,1),(i/64,1),( (i+1)/64,1),((i+1)/64,1)],0)
    stemheight = rimheight + radius*.05
    def stem(t,a):
        st=t*stemheight/(rimheight or .001)
        cx,cy=center(min(st,1))
        bulge=(1.05+.20*math.exp(-((t-.13)/.18)**2)-.22*t)
        if family==1: bulge=1-.30*t+.12*math.sin(t*math.pi)
        r=stem_radius*bulge*(1+.035*math.sin(6*a+phase)+.017*math.sin(11*a+t*12))
        return (cx+r*math.cos(a),cy+r*math.sin(a),t*stemheight)
    st=surface(12,24,stem,1,False)
    for row,flip in [(0,True),(12,False)]:
        z=row/12*stemheight; coords=[verts[st+row*24+i] for i in range(24)]
        ci=len(verts); verts.append(tuple(sum(p[k] for p in coords)/24 for k in range(3)))
        for i in range(24):
            ids=[ci,st+row*24+i,st+row*24+(i+1)%24]
            uv=[(.5,row/12),(i/24,row/12),((i+1)/24,row/12)]
            if flip: ids.reverse();uv.reverse()
            add_face(ids,uv,1)
    mesh=bpy.data.meshes.new(name+'_Mesh');mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(name,mesh); assets.objects.link(obj)
    obj.parent=parent;obj.location=(xy[0],xy[1],0)
    for m in (caps[family],stemmat,gillmat):mesh.materials.append(m)
    uv=mesh.uv_layers.new(name='UVMap')
    for poly,coords,mi in zip(mesh.polygons,uvfaces,mats):
        poly.material_index=mi;poly.use_smooth=True
        for li,coord in zip(poly.loop_indices,coords):uv.data[li].uv=coord
    # Merge coincident rim vertices and recalculate outward surface normals.
    import bmesh
    bm=bmesh.new();bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(mesh);bm.free();mesh.update()
    obj['asset_role']='Raised mushroom mesh; generic visual form'
    obj['nominal_height_m']=height
    return obj

layouts=[
 ('Chestnut',(-.19,.16),[(.155,.046,(-.045,.015)),(.115,.038,(.043,.012)),(.06,.023,(-.065,-.025))]),
 ('Bell',(.19,.16),[(.13,.024,(0,.015)),(.10,.021,(.052,.006)),(.085,.02,(-.05,.003)),(.067,.017,(.073,-.035)),(.093,.021,(-.013,-.036))]),
 ('Ivory',(-.19,-.16),[(.13,.054,(-.045,.006)),(.095,.046,(.062,-.01))]),
 ('Ochre',(.19,-.16),[(.09,.047,(-.032,.025)),(.078,.040,(.057,.015)),(.056,.029,(-.062,-.036)),(.042,.02,(.02,-.036))])]
models=[];groups=[]
for fam,(name,loc,items) in enumerate(layouts):
    root=bpy.data.objects.new(name+'_Group',None);assets.objects.link(root);root.location=(*loc,0);groups.append(root)
    for i,(h,r,xy) in enumerate(items):
        lean=(random.uniform(-.008,.008),random.uniform(-.004,.004))
        models.append(mushroom(name+'_'+str(i+1).zfill(2),fam,h,r,lean,fam*100+i,root,xy))

# Presentation elements are kept separate and are not included in the game export.
bpy.ops.mesh.primitive_plane_add(size=200, location=(0,0,-.0007));floor=bpy.context.object;floor.name='Preview Ground';move_to(floor,stage)
mat=bpy.data.materials.new('Preview Ground');mat.use_nodes=True
mat.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.055,.065,.075,1)
mat.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.95
floor.data.materials.append(mat)
def aim(obj, target): obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(0,-1.2,.65));cam=bpy.context.object;move_to(cam,stage);aim(cam,(0,0,.05));cam.data.type='ORTHO';cam.data.ortho_scale=.75;cam.data.lens=55;scene.camera=cam
for name,loc,power,size in [('Key',(-.45,-.45,.85),8,.65),('Fill',(.5,-.1,.6),4,.55),('Rim',(0,.55,.7),6,.5)]:
    bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.name=name;move_to(light,stage);light.data.energy=power;light.data.shape='DISK';light.data.size=size;aim(light,(0,0,.03))
scene.world.use_nodes=True
scene.world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.45,.48,.52,1)
scene.world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.22
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.render.resolution_x=1400;scene.render.resolution_y=1400;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(OUT/'raised_mushrooms_preview.png')

bpy.ops.object.select_all(action='DESELECT')
for obj in models+groups:obj.select_set(True)
bpy.context.view_layer.objects.active=models[0]
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_distance=.9
            area.spaces.active.region_3d.view_location=(0,0,.05)
            area.spaces.active.shading.type='MATERIAL'
            area.spaces.active.region_3d.view_perspective='CAMERA'

glb=OUT/'raised_mushrooms.glb'
bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',use_selection=True,export_apply=True,export_yup=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'raised_mushrooms.blend'))
manifest={'blender_version':bpy.app.version_string,'units':'meters','mesh_count':len(models),'groups':{},'total_triangles':0,'texture_size':512,'reference_atlas':'../../../imagegen/mushrooms/raised_mushrooms_v003_clean_alpha.png','notes':['Original modeled interpretation of atlas; not photogrammetry.','Caps and stems are separate intersecting closed shells within each mushroom mesh.','UVs and six shared base-color texture images included.','No collision, LODs or runtime integration.']}
for obj in models:
    obj.data.calc_loop_triangles();tri=len(obj.data.loop_triangles);manifest['total_triangles']+=tri
    manifest['groups'].setdefault(obj.parent.name,[]).append({'name':obj.name,'triangles':tri,'height_m':obj['nominal_height_m']})
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
bpy.ops.render.render(write_still=True)
# Independently round-trip the GLB in a temporary scene and check exported objects.
check=bpy.data.scenes.new('Export Validation');bpy.context.window.scene=check
bpy.ops.import_scene.gltf(filepath=str(glb))
imported=[o for o in check.objects if o.type=='MESH']
validation={'imported_mesh_count':len(imported),'all_meshes_have_uvs':all(len(o.data.uv_layers)>0 for o in imported),'all_meshes_have_materials':all(len(o.data.materials)>0 for o in imported),'glb_bytes':glb.stat().st_size,'expected_mesh_count':14}
assert len(imported)==14,validation
assert validation['all_meshes_have_uvs'] and validation['all_meshes_have_materials'],validation
(OUT/'validation.json').write_text(json.dumps(validation,indent=2))
print('MUSHROOM_BUILD_COMPLETE '+json.dumps(manifest)+' '+json.dumps(validation))
