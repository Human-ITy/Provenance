"""Reference-inspired luminous mushrooms. Run with Blender 5.2 in background."""
import bpy, bmesh, math, random, json, struct
from pathlib import Path
from mathutils import Vector, noise
import numpy as np

OUT=Path(__file__).resolve().parent
random.seed(903)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.unit_settings.system='METRIC'
assets=bpy.data.collections.new('Magical Mushroom Meshes');scene.collection.children.link(assets)
lights=bpy.data.collections.new('Optional Local Glow Lights');scene.collection.children.link(lights)
stage=bpy.data.collections.new('Preview Stage - not exported');scene.collection.children.link(stage)
refs=bpy.data.collections.new('Reference Cutouts - not exported');scene.collection.children.link(refs)
refs.hide_render=True;refs.hide_viewport=True

def move(obj,col):
    for c in list(obj.users_collection):c.objects.unlink(obj)
    col.objects.link(obj)

def image(name,rgb):
    h,w=rgb.shape[:2]
    img=bpy.data.images.new(name,width=w,height=h,alpha=False)
    data=np.concatenate((np.clip(rgb,0,1),np.ones((h,w,1))),axis=2).astype(np.float32)
    img.pixels.foreach_set(data.ravel());img.filepath_raw=str(OUT/(name+'.png'));img.file_format='PNG';img.save();img.pack()
    return img

def mat(name,base,kind,emission=0,emitcolor=None):
    n=512;u,v=np.meshgrid(np.arange(n)/n,np.arange(n)/n)
    x=v*np.cos(u*math.tau);y=v*np.sin(u*math.tau)
    rng=np.random.default_rng(sum(map(ord,name)))
    h=np.zeros((n,n))
    for k in range(35):
        fx,fy=rng.uniform(1,70,2);phase=rng.uniform(0,math.tau)
        if 'stem' in kind:h+=np.sin(u*math.tau*round(fx)+v*fy*.45+phase)/(1+k*.13)
        else:h+=np.sin(x*fx+y*fy+phase)/(1+k*.13)
    h/=7
    speck=np.maximum(0,np.sin(x*423+y*185)*np.sin(y*363-x*259)-.72)
    if kind=='cyan_cap':
        tone=.85+.35*h+.10*np.cos(u*math.tau*64)*v**2
        rgb=np.array(base)[None,None,:]*tone[:,:,None]
        dots=np.zeros_like(u)
        for _ in range(380):
            px,py=rng.uniform(-1,1,2);width=rng.uniform(.0015,.0045)
            dots=np.maximum(dots,np.exp(-((x-px)**2+(y-py)**2)/(2*width*width))*rng.uniform(.3,.95))
        rgb=rgb*(1-dots[:,:,None]) + np.array([.16,.88,1])[None,None,:]*dots[:,:,None]
    elif kind=='violet_cap':
        tone=.9+.35*h
        rgb=np.array(base)[None,None,:]*tone[:,:,None]
        mask=np.clip((h-.17)*3,0,.8)
        rgb=rgb*(1-mask[:,:,None])+np.array([.29,.055,.10])[None,None,:]*mask[:,:,None]
        rgb+=speck[:,:,None]*np.array([.55,.22,.05])
    elif 'gills' in kind:
        rib=.5+.5*np.cos(u*math.tau*(64 if 'cyan' in kind else 48))
        tone=.65+.35*rib+.09*h
        rgb=np.array(base)[None,None,:]*tone[:,:,None]
    else:
        tone=.9+.19*h
        if 'violet' in kind:tone+=.10*np.sin(v*180+u*12)*np.sin(u*97-v*37)
        rgb=np.array(base)[None,None,:]*tone[:,:,None]
    col=image(name+'_BaseColor',rgb)
    # A tangent-space normal map from the authored fine surface field.
    dx=(np.roll(h,-1,1)-np.roll(h,1,1))*1.2
    dy=(np.roll(h,-1,0)-np.roll(h,1,0))*1.2
    norm=np.stack((-dx,-dy,np.ones_like(h)),axis=2)
    norm/=np.linalg.norm(norm,axis=2)[:,:,None]
    normal=image(name+'_Normal',norm*.5+.5);normal.colorspace_settings.name='Non-Color'
    m=bpy.data.materials.new(name);m.use_nodes=True
    nodes=m.node_tree.nodes;bs=nodes.get('Principled BSDF')
    bs.inputs['Roughness'].default_value=.48 if 'cyan' in kind else .73
    tex=nodes.new('ShaderNodeTexImage');tex.image=col
    m.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
    tn=nodes.new('ShaderNodeTexImage');tn.image=normal
    nm=nodes.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.15 if 'cyan' in kind else .48
    m.node_tree.links.new(tn.outputs['Color'],nm.inputs['Color']);m.node_tree.links.new(nm.outputs['Normal'],bs.inputs['Normal'])
    if emission:
        bs.inputs['Emission Color'].default_value=(*(emitcolor or base),1)
        bs.inputs['Emission Strength'].default_value=emission
        if 'stem' in kind or 'gills' in kind:
            m.node_tree.links.new(tex.outputs['Color'],bs.inputs['Emission Color'])
    return m

def plain(name,color,emit=0):
    m=bpy.data.materials.new(name);m.use_nodes=True
    b=m.node_tree.nodes.get('Principled BSDF');b.inputs['Base Color'].default_value=(*color,1);b.inputs['Roughness'].default_value=.45
    b.inputs['Emission Color'].default_value=(*color,1);b.inputs['Emission Strength'].default_value=emit
    return m

cyan_cap=mat('Cyan_Cap',(.025,.22,.53),'cyan_cap',.06,(.01,.18,.55))
cyan_stem=mat('Cyan_Stem',(.055,.61,.75),'cyan_stem',.42,(.015,.65,1))
cyan_gill=mat('Cyan_Gills',(.06,.63,.83),'cyan_gills',.95,(.025,.72,1))
cyan_glow=plain('Cyan_Luminous_Filaments',(.025,.70,1),1.65)
violet_cap=mat('Violet_Cap',(.255,.145,.48),'violet_cap')
violet_stem=mat('Violet_Stem',(.19,.07,.30),'violet_stem',.015,(.25,.025,.15))
violet_gill=mat('Warm_Gills',(.80,.18,.055),'warm_gills',.80,(1,.19,.035))
warm_vein=plain('Warm_Gill_Edges',(1,.40,.08),1.8)
skirtmat=mat('Violet_Skirt',(.29,.045,.125),'violet_stem')
wartmat=plain('Violet_Cap_Scales',(.14,.045,.12))

def meshgrid(name,rows,segs,fn,material,reverse=False,close_ends=False):
    vs=[];fs=[];ufs=[]
    for j in range(rows+1):
        for i in range(segs):vs.append(fn(j/rows,i/segs*math.tau))
    for j in range(rows):
        for i in range(segs):
            ids=[j*segs+i,j*segs+(i+1)%segs,(j+1)*segs+(i+1)%segs,(j+1)*segs+i]
            uv=[(i/segs,j/rows),((i+1)/segs,j/rows),((i+1)/segs,(j+1)/rows),(i/segs,(j+1)/rows)]
            if reverse:ids.reverse();uv.reverse()
            fs.append(ids);ufs.append(uv)
    if close_ends:
        for row,flip in [(0,True),(rows,False)]:
            points=vs[row*segs:(row+1)*segs];idx=len(vs)
            vs.append(tuple(sum(p[k] for p in points)/segs for k in range(3)))
            for i in range(segs):
                face=[idx,row*segs+i,row*segs+(i+1)%segs];uv=[(.5,row/rows),(i/segs,row/rows),((i+1)/segs,row/rows)]
                if flip:face.reverse();uv.reverse()
                fs.append(face);ufs.append(uv)
    me=bpy.data.meshes.new(name);me.from_pydata(vs,[],fs);me.update()
    ob=bpy.data.objects.new(name,me);assets.objects.link(ob);me.materials.append(material)
    uv=me.uv_layers.new(name='UVMap')
    for f,coords in zip(me.polygons,ufs):
        f.use_smooth=True
        for li,co in zip(f.loop_indices,coords):uv.data[li].uv=co
    return ob

def tube(name,points,radius,material):
    cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.resolution_u=1;cu.bevel_depth=radius;cu.bevel_resolution=2;cu.use_fill_caps=True
    sp=cu.splines.new('POLY');sp.points.add(len(points)-1)
    for p,co in zip(sp.points,points):p.co=(*co,1)
    ob=bpy.data.objects.new(name,cu);assets.objects.link(ob);cu.materials.append(material)
    return ob

def bead(name,co,r,material,scale=(1,1,1)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=8,ring_count=4,radius=r,location=co)
    ob=bpy.context.object;ob.name=name;ob.scale=scale;move(ob,assets);ob.data.materials.append(material)
    for p in ob.data.polygons:p.use_smooth=True
    return ob

def join_parts(parts,name,root):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in parts:ob.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.convert(target='MESH');bpy.ops.object.join()
    ob=bpy.context.object;ob.name=name
    # Origin at the ground contact point; all component coordinates already local.
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    ob.parent=root
    return ob

def mushroom(name,H,R,kind,lean,root,loc,seed):
    rng=random.Random(seed);parts=[]
    violet=kind=='violet';rim=H-R*(.62 if violet else .72)
    def axis(t):return (lean[0]*(math.sin(t*math.pi*.65)**1.5),lean[1]*t*t)
    cx,cy=axis(1)
    def outline(a):return 1+(.065 if violet else .018)*math.sin(5*a+.6)+.015*math.sin(9*a)
    def wav(a):return R*(.070 if violet else .009)*math.sin(6*a+.5)+R*(.025 if violet else .004)*math.cos(9*a)
    def cap(t,a):
        rr=R*(.0001+.9999*t)*outline(a)
        z=rim+R*(.62 if violet else .72)*max(0,1-t*t)**(.72 if violet else .64)
        z+=wav(a)*t**3
        z-=rr*math.sin(a)*(.22 if violet else .26)
        if violet:z+=.001*noise.noise(Vector((rr*math.cos(a)*80,rr*math.sin(a)*80,1.4)))
        else:z+=R*.0025*math.sin(a*64)*t**2
        return (cx+rr*math.cos(a),cy+rr*math.sin(a),z)
    parts.append(meshgrid(name+'_Cap',32,128,cap,violet_cap if violet else cyan_cap,True))
    # Shallow gill fans ripple down from the underside; violet has broader pleats.
    def under(t,a):
        rr=R*(.015+.985*t)*outline(a)
        rib=(.5+.5*math.cos((48 if violet else 64)*a))
        dip=R*(.18 if violet else .14)*(math.sin(math.pi*t)**.7)
        z=rim-R*.02*(1-t)-dip*(.25+.75*rib)+wav(a)*t**3
        z-=rr*math.sin(a)*(.22 if violet else .26)
        return (cx+rr*math.cos(a),cy+rr*math.sin(a),z)
    parts.append(meshgrid(name+'_Gills',14,192 if violet else 128,under,violet_gill if violet else cyan_gill))
    # A softly curved stem, widened at the base and tucked into the cap.
    sh=rim+R*.04
    sr=R*(.14 if violet else .18)
    def stem(t,a):
        x,y=axis(t)
        rr=sr*(1.12-.33*t+.18*math.exp(-((t-.12)/.22)**2))
        if violet:rr*=1+.065*math.sin(t*170+a*9)+.032*math.cos(t*95-a*17)
        else:rr*=1+.018*math.sin(a*28+t*19)
        return (x+rr*math.cos(a),y+rr*math.sin(a),t*sh)
    parts.append(meshgrid(name+'_Stem',48,48,stem,violet_stem if violet else cyan_stem,False,True))
    if violet:
        # Ragged, hanging annular skirt below the cap.
        def skirt(t,a):
            rr=sr*(.85+.88*t+.10*math.sin(16*a)*t)
            z=sh-.015-t*.049+(math.sin(a*11)*.003+math.sin(a*23)*.0015)*t**3
            return (cx+rr*math.cos(a),cy+rr*math.sin(a),z)
        parts.append(meshgrid(name+'_Ragged_Skirt',12,96,skirt,skirtmat))
        # Slender warm outlines follow each gill from the stem toward the scalloped rim.
        for i in range(48):
            a=math.tau*i/48
            pts=[]
            for j in range(25):
                t=.17+.82*j/24;co=under(t,a+.028*math.sin(t*math.pi));pts.append((co[0],co[1],co[2]-.00025))
            parts.append(tube(name+'_Gill_Vein',pts,.00035,warm_vein))
        for i in range(92):
            t=math.sqrt(rng.uniform(.015,.90));a=rng.uniform(0,math.tau);co=cap(t,a)
            b=bead(name+'_Cap_Scale',co,rng.uniform(.0013,.0031),wartmat,(1.4,1,.27))
            b.rotation_euler[2]=a;parts.append(b)
    else:
        for i in range(9 if H>.23 else 5):
            a=math.tau*(i/(9 if H>.23 else 5))+.23
            co=under(.80,a);length=R*rng.uniform(.45,.8)
            pts=[]
            for j in range(45):
                t=j/44
                curl=max(0,(t-.55)/.45)
                x=co[0]+.003*math.sin(t*8+i)+R*.055*curl*math.sin(curl*math.tau*1.25)
                y=co[1]+.003*math.sin(t*5+i)
                z=co[2]-length*t+R*.045*curl*math.cos(curl*math.tau*1.25)
                pts.append((x,y,z))
            parts.append(tube(name+'_Hanging_Filament',pts,R*.004,cyan_glow))
            parts.append(bead(name+'_Filament_Drop',pts[-1],R*.015,cyan_glow,(.85,.85,1.2)))
        for i in range(70 if H>.23 else 35):
            t=math.sqrt(rng.uniform(.01,.97));a=rng.uniform(0,math.tau)
            parts.append(bead(name+'_Cap_Glow_Dot',cap(t,a),R*rng.uniform(.0025,.006),cyan_glow))
    ob=join_parts(parts,name,root);ob.location=(*loc,0)
    ob['height_m']=H;ob['reference_design']='Violet warm-gill reference' if violet else 'Cyan luminous pair reference'
    ob['emission_note']='Low constant emission; apparent brightness increases in darkness. No automatic day/night logic.'
    bpy.ops.object.light_add(type='POINT',location=(0,0,0));light=bpy.context.object;move(light,lights);light.name=name+'_Optional_Local_Glow';light.parent=ob
    light.location=(cx,cy,rim*.65);light.data.energy=.018 if violet else .012;light.data.color=(1,.20,.045) if violet else (.02,.55,1);light.data.shadow_soft_size=.025
    light['role']='Optional nearby-ground illumination; may be replaced by engine light budget.'
    return ob

roots=[]
for name,pos in [('Cyan_Pair',(-.20,0,0)),('Violet_Warm_Gill',(.22,.012,0))]:
    root=bpy.data.objects.new(name,None);assets.objects.link(root);root.location=pos;roots.append(root)
models=[mushroom('Cyan_Tall',.29,.095,'cyan',(.027,.003),roots[0],(.028,.022),18),
        mushroom('Cyan_Young',.18,.059,'cyan',(-.036,0),roots[0],(-.082,-.015),22),
        mushroom('Violet_Lantern',.315,.145,'violet',(.016,0),roots[1],(0,0),31)]

# Pack the user's originals and the extracted references in the editable project.
for idx,key in enumerate(('cyan','violet')):
    for variant in ('original','cutout'):
        p=OUT/'references'/(key+'_'+variant+'.png')
        img=bpy.data.images.load(str(p));img.pack()
        ob=bpy.data.objects.new(key+'_'+variant+'_Reference',None);refs.objects.link(ob)
        ob.empty_display_type='IMAGE';ob.data=img;ob.empty_display_size=.35;ob.location=(idx*.45,.5,.2)

def aim(ob,pt):ob.rotation_euler=(Vector(pt)-ob.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.001));floor=bpy.context.object;move(floor,stage);floor.name='Preview Ground'
floor.data.materials.append(plain('Preview Ground',(.035,.045,.065)))
bpy.ops.object.camera_add(location=(0,-1.15,.34));camera=bpy.context.object;move(camera,stage);aim(camera,(0,0,.155));camera.data.type='ORTHO';camera.data.ortho_scale=.84;scene.camera=camera
studio=[]
for name,loc,power,size in [('Key',(-.5,-.55,.85),11,.7),('Fill',(.55,-.3,.65),6,.55),('Rim',(0,.6,.7),8,.45)]:
    bpy.ops.object.light_add(type='AREA',location=loc);ob=bpy.context.object;move(ob,stage);ob.name=name;ob.data.energy=power;ob.data.shape='DISK';ob.data.size=size;aim(ob,(0,0,.13));studio.append(ob)
scene.world.use_nodes=True;bg=scene.world.node_tree.nodes.get('Background');bg.inputs['Color'].default_value=(.25,.32,.5,1)
scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True
scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX';scene.render.image_settings.file_format='PNG'

def set_lighting(night):
    bg.inputs['Strength'].default_value=.012 if night else .35
    for ob,power in zip(studio,([.12,.06,.22] if night else [11,6,8])):ob.data.energy=power

def select_export(objs):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objs:ob.select_set(True)
    bpy.context.view_layer.objects.active=models[0]

select_export(models+roots+list(lights.objects))
bpy.ops.export_scene.gltf(filepath=str(OUT/'magical_mushrooms.glb'),export_format='GLB',use_selection=True,export_apply=True,export_yup=True,export_lights=True)
for root,name in zip(roots,['cyan_pair.glb','violet_lantern.glb']):
    children=[ob for ob in models if ob.parent==root];extras=[ob for ob in lights.objects if ob.parent in children]
    old=root.location.copy();root.location=(0,0,0)
    select_export([root]+children+extras)
    bpy.ops.export_scene.gltf(filepath=str(OUT/name),export_format='GLB',use_selection=True,export_apply=True,export_yup=True,export_lights=True)
    root.location=old
select_export(models)
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_perspective='CAMERA';area.spaces.active.shading.type='MATERIAL'
            area.spaces.active.shading.use_scene_lights=True;area.spaces.active.shading.use_scene_world=True
set_lighting(True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'magical_mushrooms.blend'))
for night,label in [(False,'daylight'),(True,'darkness')]:
    set_lighting(night);scene.render.filepath=str(OUT/('magical_mushrooms_'+label+'.png'));bpy.ops.render.render(write_still=True)
manifest={'blender':bpy.app.version_string,'units':'meters','models':[],'notes':['Reference-inspired original meshes, not exact image reconstruction.','Low constant emissive materials plus optional local point lights. No automatic darkness detection.','Preview lighting excluded from exports.','No collision, LOD or game integration.']}
for ob in models:
    ob.data.calc_loop_triangles();manifest['models'].append({'name':ob.name,'triangles':len(ob.data.loop_triangles),'height_m':ob['height_m'],'materials':[m.name for m in ob.data.materials]})
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
# Validate embedded PBR textures, emission and punctual lights in the actual GLB JSON.
data=(OUT/'magical_mushrooms.glb').read_bytes();size=struct.unpack_from('<I',data,12)[0];doc=json.loads(data[20:20+size])
valid={'mesh_count':len(doc.get('meshes',[])),'materials':len(doc.get('materials',[])),'embedded_images':len(doc.get('images',[])),'emissive_materials':sum(bool(m.get('emissiveFactor')) for m in doc.get('materials',[])),'punctual_lights':len(doc.get('extensions',{}).get('KHR_lights_punctual',{}).get('lights',[])),'extensions_used':doc.get('extensionsUsed',[]),'bytes':len(data)}
assert valid['mesh_count']==3 and valid['emissive_materials']>=4 and valid['punctual_lights']==3,valid
check=bpy.data.scenes.new('GLB Validation');bpy.context.window.scene=check;bpy.ops.import_scene.gltf(filepath=str(OUT/'magical_mushrooms.glb'))
valid['roundtrip_meshes']=len([o for o in check.objects if o.type=='MESH']);assert valid['roundtrip_meshes']==3
(OUT/'validation.json').write_text(json.dumps(valid,indent=2));print('MAGICAL_MUSHROOMS_COMPLETE '+json.dumps(manifest)+' '+json.dumps(valid))
