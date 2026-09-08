"""Reference-led hammer authoring; metre-scale static game mesh and preview."""
import bpy,math,json,random
from mathutils import Vector
from pathlib import Path
P=Path(__file__).resolve().parent;P.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene;scene.unit_settings.system='METRIC'
def material(name,stem,metal,rough):
    m=bpy.data.materials.new(name);m.use_nodes=True;bs=m.node_tree.nodes['Principled BSDF'];bs.inputs['Metallic'].default_value=metal;bs.inputs['Roughness'].default_value=rough
    for kind,socket in [('color','Base Color'),('normal','Normal')]:
        tx=m.node_tree.nodes.new('ShaderNodeTexImage');tx.image=bpy.data.images.load(str(P/'textures'/(stem+'_'+kind+'.png')));tx.image.pack()
        if kind=='normal':
            tx.image.colorspace_settings.name='Non-Color';nm=m.node_tree.nodes.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.4;m.node_tree.links.new(tx.outputs['Color'],nm.inputs['Color']);m.node_tree.links.new(nm.outputs[0],bs.inputs[socket])
        else:m.node_tree.links.new(tx.outputs['Color'],bs.inputs[socket])
    return m
iron=material('Forged dark iron','iron',.88,.48);gold=material('Worn brass','brass',.82,.38);wood=material('Walnut shaft','wood',0,.65);leather=material('Dark leather','leather',0,.72)
objects=[]
def finish(o,name,mat,bevel=0):
    o.name=name;o.data.materials.append(mat);bpy.context.view_layer.objects.active=o
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Forged edge roundover','BEVEL');mod.width=bevel;mod.segments=3
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.02);bpy.ops.object.mode_set(mode='OBJECT')
    for p in o.data.polygons:p.use_smooth=False
    if bevel:
        mod=o.modifiers.new('Weighted face normals','WEIGHTED_NORMAL');mod.keep_sharp=True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    objects.append(o);return o
def box(name,loc,scale,mat,bevel=.003):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=scale;return finish(o,name,mat,bevel)
def cyl(name,z,r,depth,mat,verts=32,r2=None):
    bpy.ops.mesh.primitive_cone_add(vertices=verts,radius1=r,radius2=r if r2 is None else r2,depth=depth,location=(0,0,z))
    return finish(bpy.context.object,name,mat,min(.0012,depth*.12))
def line(name,points,mat,width=.0006,closed=False):
    cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.resolution_u=1;cu.bevel_depth=width;cu.bevel_resolution=2
    sp=cu.splines.new('POLY');sp.points.add(len(points)-1)
    for p,v in zip(sp.points,points):p.co=(*v,1)
    sp.use_cyclic_u=closed;o=bpy.data.objects.new(name,cu);scene.collection.objects.link(o)
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH');return finish(o,name,mat)
def diamond(name,x,y,z,size,mat):
    return line(name,[(x,y,z+size),(x+size*.58,y,z),(x,y,z-size),(x-size*.58,y,z)],mat,.0007,True)
# Substantial block, inset socket housing, and forged tapered peen.
box('Striking block',(-.123,0,.566),(.133,.112,.137),iron,.007)
box('Central eye housing',(-.014,0,.570),(.116,.118,.127),iron,.005)
verts=[]
for x,hy,hz,z in [(.045,.055,.059,.572),(.199,.031,.025,.580)]:
    verts += [(x,-hy,z-hz),(x,hy,z-hz),(x,hy,z+hz),(x,-hy,z+hz)]
me=bpy.data.meshes.new('Tapered peen');me.from_pydata(verts,[],[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]);me.update();o=bpy.data.objects.new('Tapered peen',me);scene.collection.objects.link(o);finish(o,'Tapered peen',iron,.004)
# Bands follow the rectangular head cross-section, not disconnected front stripes.
for x,hy,hz,z in [(-.127,.058,.070,.566),(.067,.054,.056,.574)]:
    # Rounded rectangular strap as broad metal strips with rounded corners.
    box('Brass head band front',(x,-hy,z),(.010,.003,hz*2),gold,.0012)
    box('Brass head band back',(x,hy,z),(.010,.003,hz*2),gold,.0012)
    box('Brass head band top',(x,0,z+hz),(.010,hy*2,.003),gold,.001)
    box('Brass head band bottom',(x,0,z-hz),(.010,hy*2,.003),gold,.001)
cyl('Upper eye locking boss',.642,.033,.013,gold,48)
cyl('Eye boss lower rim',.636,.036,.004,gold,48)
cyl('Socket collar',.486,.028,.050,gold,40,r2=.032)
for z in [.464,.507]:cyl('Collar raised rim',z,.030,.004,gold)
# Wood is a continuous shaft passing into the head socket.
cyl('Walnut shaft',.303,.018,.371,wood,32,r2=.020)
cyl('Leather grip core',.130,.021,.214,leather,32)
# Helical leather ribbon, slightly overlapping, with shallow raised binding edges.
v=[];f=[];turns=7;steps=turns*48
for i in range(steps+1):
    t=i/steps;a=t*turns*math.tau;z=.023+t*.202
    for dz in [-.012,.012]:v.append((.022*math.cos(a),.022*math.sin(a),z+dz))
for i in range(steps):f.append((2*i,2*i+1,2*i+3,2*i+2))
me=bpy.data.meshes.new('Leather wrap');me.from_pydata(v,[],f);me.update();o=bpy.data.objects.new('Spiral leather wrap',me);scene.collection.objects.link(o);finish(o,'Spiral leather wrap',leather)
for edge in [-.012,.012]:line('Wrap worn lip',[(.0223*math.cos(i/steps*turns*math.tau),.0223*math.sin(i/steps*turns*math.tau),.023+i/steps*.202+edge) for i in range(steps+1)],leather,.0006)
for z in [.022,.240]:cyl('Grip brass binding',z,.024,.010,gold,40)
cyl('Faceted pommel',.010,.030,.035,gold,10,r2=.024)
cyl('Pommel base cap',-.008,.025,.006,iron,10)
# Physical brass inlays, both visible cheeks; unseen back design is inferred.
for sign in [-1,1]:
    y=sign*.060
    line('Triangular guild inlay',[(-.056,y,.535),(.024,y,.535),(-.014,y,.604)],gold,.0007,True)
    line('Guild vertical',[(-.014,y,.522),(-.014,y,.619)],gold,.00065)
    diamond('Central guild diamond',-.014,y,.566,.009,gold)
    for j in range(5):line('Graduated ticks',[(-.070,sign*.057,.526+j*.015),(-.061,sign*.057,.526+j*.015)],gold,.0006)
    y=sign*.059
    line('Striking-face circle',[(-.128+.013*math.cos(j*math.tau/48),y,.564+.013*math.sin(j*math.tau/48)) for j in range(48)],gold,.0007,True)
    line('Circle vertical',[(-.128,y,.526),(-.128,y,.602)],gold,.00065)
    line('Circle horizontal',[(-.151,y,.564),(-.105,y,.564)],gold,.00065)
    diamond('Collar lozenge',0,sign*.0298,.483,.010,iron)
    diamond('Pommel lozenge',0,sign*.027,.009,.011,iron)
    for j in range(5):line('Shaft score',[(-.003,sign*.0204,.413-j*.012),(.006,sign*.0204,.413-j*.012)],iron,.00035)
    diamond('Shaft lozenge',.001,sign*.0202,.347,.008,iron)
for y in [-.009,0,.009]:line('Boss engraved stripe',[(-.019,y,.650),(.019,y,.650)],iron,.00065)
# Origin at pommel base for ground placement; same frame for all exports.
for o in objects:o.location.z+=.011
bpy.context.view_layer.update()
parts=[]
for mat in [iron,gold,wood,leather]:
    chosen=[o for o in list(scene.objects) if o.type=='MESH' and o.data.materials[0]==mat]
    bpy.ops.object.select_all(action='DESELECT')
    for o in chosen:o.select_set(True)
    bpy.context.view_layer.objects.active=chosen[0];bpy.ops.object.join();o=bpy.context.object;o.name='Hammer_'+mat.name.replace(' ','_')
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);parts.append(o)
def export(path,obs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in obs:o.select_set(True)
    bpy.context.view_layer.objects.active=obs[0];bpy.ops.export_scene.gltf(filepath=str(path),export_format='GLB',use_selection=True,export_extras=True,export_yup=True,export_apply=True,export_cameras=False,export_lights=False)
export(P/'guild_hammer_lod0.glb',parts)
lod=[]
for o in parts:
    cp=o.copy();cp.data=o.data.copy();scene.collection.objects.link(cp);bpy.context.view_layer.objects.active=cp
    mod=cp.modifiers.new('Secondary detail','DECIMATE');mod.ratio=.55;bpy.ops.object.modifier_apply(modifier=mod.name);lod.append(cp)
export(P/'guild_hammer_lod1.glb',lod)
for o in lod:bpy.data.objects.remove(o,do_unlink=True)
points=[o.matrix_world@v.co for o in parts for v in o.data.vertices];bounds=[[min(p[k] for p in points) for k in range(3)],[max(p[k] for p in points) for k in range(3)]]
anchors={'grip':[0,0,.143],'primary_strike':[-.191,0,.577],'peen_strike':[.200,0,.591],'pickup_center':[0,0,.36]}
for name,pos in anchors.items():
    e=bpy.data.objects.new(name,None);scene.collection.objects.link(e);e.location=pos;e.empty_display_size=.015;e['role']=name
bpy.ops.wm.save_as_mainfile(filepath=str(P/'GuildHammer.blend'))
def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(-.90,-1.65,.96));cam=bpy.context.object;aim(cam,(0,0,.34));cam.data.type='ORTHO';cam.data.ortho_scale=.86;scene.camera=cam
for loc,power,size in [((-.6,-1,1.5),90,1),((.8,-.2,.9),55,.7),((0,1,1.1),100,.8)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.size=size;aim(o,(0,0,.36))
world=bpy.data.worlds.new('Studio');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[1].default_value=.35;scene.world=world
scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True;scene.render.film_transparent=True;scene.render.image_settings.color_mode='RGBA';scene.view_settings.view_transform='AgX'
scene.render.resolution_x=1000;scene.render.resolution_y=1200;scene.render.resolution_percentage=100;scene.render.filepath=str(P/'hammer_review.png');bpy.ops.render.render(write_still=True)
scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.filepath=str(P/'hammer_inventory_icon.png');bpy.ops.render.render(write_still=True)
tri=sum((o.data.calc_loop_triangles() or len(o.data.loop_triangles)) for o in parts)
manifest={'asset_id':'guild_hammer','units':'metres','blender_up':'Z','gltf_up':'Y','bounds_z_up':bounds,'anchors_z_up':anchors,'visual_lods':['guild_hammer_lod0.glb','guild_hammer_lod1.glb'],'lod0_triangles':tri,'reference_interpretation':'Original supplied hammer silhouette and decorative details; back/hidden construction inferred. One-handed authored scale, not calibrated from photo.','pickup':'E when aimed within 2m, one item in the current Play session; see runtime handoff.'}
(P/'manifest.json').write_text(json.dumps(manifest,indent=2));print('HAMMER_MODEL_COMPLETE',tri,bounds)
