"""Two fictional berry bushes with individually detachable fruit. Blender 5.2."""
import bpy, bmesh, math, random, json, struct
import numpy as np
from pathlib import Path
from mathutils import Vector

OUT=Path(__file__).resolve().parent
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.unit_settings.system='METRIC'
assets=bpy.data.collections.new('Berry Bush Assets');scene.collection.children.link(assets)
samples=bpy.data.collections.new('Standalone Fruit Samples');scene.collection.children.link(samples)
stage=bpy.data.collections.new('Preview Stage - excluded from export');scene.collection.children.link(stage)

def move(obj,col):
    for c in list(obj.users_collection):c.objects.unlink(obj)
    col.objects.link(obj)

def image(name,rgb):
    h,w=rgb.shape[:2];im=bpy.data.images.new(name,width=w,height=h,alpha=False)
    rgba=np.concatenate((np.clip(rgb,0,1),np.ones((h,w,1))),axis=2).astype(np.float32)
    im.pixels.foreach_set(rgba.ravel());im.filepath_raw=str(OUT/(name+'.png'));im.file_format='PNG';im.save();im.pack();return im

def material(name,base,kind,seed):
    rng=np.random.default_rng(seed);n=512;u,v=np.meshgrid(np.arange(n)/n,np.arange(n)/n)
    noise=np.zeros((n,n))
    for k in range(20):
        fx,fy=rng.integers(2,65,2);noise+=np.sin(math.tau*(fx*u+fy*v)+rng.uniform(0,math.tau))/(1+k*.2)
    noise/=5
    if kind=='leaf':
        x=np.abs(u-.5)*2
        sideveins=(.5+.5*np.cos((v-x*.24)*math.tau*9))**25
        vein=np.exp(-((u-.5)/.012)**2)+.28*sideveins
        tone=.86+.19*noise+.12*v+.21*vein-.10*x*x
        rgb=np.array(base)[None,None,:]*tone[:,:,None]
        rgb+=vein[:,:,None]*np.array([.025,.035,.005])
    elif kind=='wood':
        tone=.8+.22*noise+.10*np.sin(u*math.tau*63+np.sin(v*30))
        rgb=np.array(base)[None,None,:]*tone[:,:,None]
    elif kind=='blue':
        bloom=np.clip(.30+.26*noise+.10*np.sin(v*7),.05,.6)
        rgb=np.array(base)[None,None,:]*(1-bloom[:,:,None])+np.array([.34,.43,.63])[None,None,:]*bloom[:,:,None]
    else:
        tone=.85+.13*noise+.09*np.cos(v*math.tau)
        rgb=np.array(base)[None,None,:]*tone[:,:,None]
    mat=bpy.data.materials.new(name);mat.use_nodes=True;mat.use_backface_culling=False
    bs=mat.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Roughness'].default_value={'red':.28,'blue':.68,'wood':.87,'leaf':.63}.get(kind,.7)
    tx=mat.node_tree.nodes.new('ShaderNodeTexImage');tx.image=image(name+'_BaseColor',rgb)
    mat.node_tree.links.new(tx.outputs['Color'],bs.inputs['Base Color'])
    if kind=='leaf':
        bs.inputs['Subsurface Weight'].default_value=.03
    return mat

wood=material('Woody_Branches',(.235,.145,.078),'wood',5)
leafred=material('Healing_Bush_Leaf',(.15,.335,.060),'leaf',7)
leafblue=material('Stamina_Bush_Leaf',(.105,.26,.16),'leaf',9)
redmat=material('Healing_Red_Berry',(.69,.025,.04),'red',11)
bluemat=material('Stamina_Blue_Berry',(.025,.055,.22),'blue',13)
def plain(name,col,rough=.75):
    m=bpy.data.materials.new(name);m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*col,1);bs.inputs['Roughness'].default_value=rough
    return m
calyxred=plain('Red_Berry_Calyx',(.08,.13,.025));calyxblue=plain('Blue_Berry_Crown',(.012,.018,.045))

def mesh(name,verts,faces,uvs,mat):
    me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update();me.materials.append(mat)
    ob=bpy.data.objects.new(name,me);assets.objects.link(ob)
    uv=me.uv_layers.new(name='UVMap')
    for poly,coords in zip(me.polygons,uvs):
        poly.use_smooth=True
        for li,co in zip(poly.loop_indices,coords):uv.data[li].uv=co
    return ob

def branch(name,points,r0,r1):
    points=[Vector(p) for p in points];verts=[];faces=[];uvs=[];seg=8
    for j,p in enumerate(points):
        tangent=(points[min(j+1,len(points)-1)]-points[max(0,j-1)]).normalized()
        axis=tangent.cross(Vector((0,1,0)))
        if axis.length<.001:axis=tangent.cross(Vector((1,0,0)))
        axis.normalize();side=tangent.cross(axis).normalized();r=r0+(r1-r0)*j/(len(points)-1)
        for i in range(seg):
            a=math.tau*i/seg;verts.append(p+r*(axis*math.cos(a)+side*math.sin(a)))
    for j in range(len(points)-1):
        for i in range(seg):
            faces.append([j*seg+i,j*seg+(i+1)%seg,(j+1)*seg+(i+1)%seg,(j+1)*seg+i])
            uvs.append([(i/seg,j/(len(points)-1)),((i+1)/seg,j/(len(points)-1)),((i+1)/seg,(j+1)/(len(points)-1)),(i/seg,(j+1)/(len(points)-1))])
    for j,rev in [(0,True),(len(points)-1,False)]:
        ci=len(verts);verts.append(points[j])
        for i in range(seg):
            f=[ci,j*seg+i,j*seg+(i+1)%seg];uv=[(.5,.5),(0,0),(1,1)]
            if rev:f.reverse();uv.reverse()
            faces.append(f);uvs.append(uv)
    return mesh(name,verts,faces,uvs,wood)

def leaf(name,base,direction,length,width,mat,rng):
    d=Vector(direction).normalized();up=Vector((0,0,1));w=d.cross(up)
    if w.length<.1:w=d.cross(Vector((0,1,0)))
    w.normalize();normal=w.cross(d).normalized();base=Vector(base)
    verts=[];faces=[];uvs=[];rows=12;cols=4
    curl=rng.uniform(-.006,.009)
    for j in range(rows+1):
        t=j/rows
        profile=max(.008,math.sin(math.pi*t)**.72)*(1-.20*t)
        serration=1 if j%2 else .90
        for k in range(cols+1):
            side=-1+2*k/cols
            p=base+d*(length*t)+w*(side*width*profile*serration)
            p+=normal*(.004*math.sin(math.pi*t)*(1-side*side)+curl*t*t-.002*side*side)
            verts.append(p)
    for j in range(rows):
        for k in range(cols):
            faces.append([j*(cols+1)+k,(j+1)*(cols+1)+k,(j+1)*(cols+1)+k+1,j*(cols+1)+k+1])
            uvs.append([(k/cols,j/rows),(k/cols,(j+1)/rows),((k+1)/cols,(j+1)/rows),((k+1)/cols,j/rows)])
    return mesh(name,verts,faces,uvs,mat)

def fruit(name,kind,r):
    verts=[];faces=[];uvs=[];seg=20;rows=12
    for j in range(1,rows):
        t=math.pi*j/rows
        for i in range(seg):
            a=math.tau*i/seg;rr=r*math.sin(t)*(1+.025*math.cos(5*a)*math.sin(t))
            verts.append((rr*math.cos(a),rr*math.sin(a)*.98,r*math.cos(t)*(1.07 if kind=='red' else .92)))
    for j in range(rows-2):
        for i in range(seg):
            faces.append([j*seg+i,j*seg+(i+1)%seg,(j+1)*seg+(i+1)%seg,(j+1)*seg+i])
            uvs.append([(i/seg,(j+1)/rows),((i+1)/seg,(j+1)/rows),((i+1)/seg,(j+2)/rows),(i/seg,(j+2)/rows)])
    for row,top in [(0,True),(rows-2,False)]:
        ci=len(verts);verts.append((0,0,r*(1.07 if kind=='red' else .92)*(1 if top else -1)))
        for i in range(seg):
            f=[ci,row*seg+i,row*seg+(i+1)%seg];uv=[(.5,0 if top else 1),(i/seg,0 if top else 1),((i+1)/seg,0 if top else 1)]
            if not top:f.reverse();uv.reverse()
            faces.append(f);uvs.append(uv)
    ob=mesh(name,verts,faces,uvs,redmat if kind=='red' else bluemat)
    # A distinct five-point calyx/crown is part of the harvested fruit mesh.
    cv=[(0,0,r*(1.065 if kind=='red' else .91))];cf=[];cu=[]
    for i in range(10):
        a=math.tau*i/10;rr=r*(.40 if i%2==0 else .17)
        cv.append((rr*math.cos(a),rr*math.sin(a),r*(1.04 if kind=='red' else .96)+(.001 if i%2==0 else 0)))
    for i in range(10):cf.append([0,1+i,1+(i+1)%10]);cu.append([(.5,.5),(0,0),(1,1)])
    crown=mesh(name+'_Calyx',cv,cf,cu,calyxred if kind=='red' else calyxblue)
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);crown.select_set(True);bpy.context.view_layer.objects.active=ob;bpy.ops.object.join()
    bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free();ob.data.update()
    return ob

def join(parts,name,root):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in parts:ob.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();ob=bpy.context.object;ob.name=name;ob.parent=root;return ob

roots=[];bodies=[];berries=[];sockets=[];specs=[]
for family,kind,effect,height,spread,worldx,seed in [('Healing_Red','red','healing',.80,.34,-.48,133),('Stamina_Blue','blue','stamina',.57,.29,.48,271)]:
    rng=random.Random(seed);root=bpy.data.objects.new(family+'_Bush',None);assets.objects.link(root);root.location=(worldx,0,0)
    root['asset_id']=family.lower()+'_bush';root['game_role']='berry_bush';root['berry_effect']=effect;root['harvest_state']='ripe';roots.append(root)
    parts=[];twigsites=[]
    trunk=[(math.sin(t*3)*.01,math.sin(t*5)*.006,height*.46*t) for t in np.linspace(0,1,13)]
    parts.append(branch(family+'_Trunk',trunk,.015,.004))
    lmat=leafred if kind=='red' else leafblue
    for bi in range(9):
        a=math.tau*bi/9+rng.uniform(-.16,.16)
        start=Vector((0,0,height*(.08+.022*bi)))
        end=Vector((math.cos(a)*spread*rng.uniform(.68,1),math.sin(a)*spread*rng.uniform(.68,1),height*rng.uniform(.63,.91)))
        def mainpoint(t):return start+(end-start)*t+Vector((math.cos(a)*.055*math.sin(math.pi*t),math.sin(a)*.055*math.sin(math.pi*t),height*.08*math.sin(math.pi*t)))
        parts.append(branch(family+'_MainBranch',[mainpoint(t) for t in np.linspace(0,1,15)],.0065,.0015))
        for ti in range(3):
            anchor=mainpoint(.37+ti*.20);ta=a+(-1 if ti%2 else 1)*rng.uniform(.50,.95)
            tip=anchor+Vector((math.cos(ta)*spread*.36,math.sin(ta)*spread*.36,height*rng.uniform(.10,.17)))
            parts.append(branch(family+'_Twig',[anchor+(tip-anchor)*t+Vector((0,0,.015*math.sin(math.pi*t))) for t in np.linspace(0,1,7)],.0023,.0008))
            twigsites.append((anchor,tip,ta))
            for li in range(5):
                t=.18+li*.18;base=anchor+(tip-anchor)*t
                la=ta+(-1 if li%2 else 1)*rng.uniform(.55,1.18)
                direction=Vector((math.cos(la),math.sin(la),rng.uniform(-.15,.70)))
                L=(.09 if kind=='red' else .070)*rng.uniform(.78,1.16)
                W=L*(.38 if kind=='red' else .27)
                petiole=base+direction.normalized()*.009
                parts.append(branch(family+'_Petiole',[base,petiole],.00065,.0004))
                parts.append(leaf(family+'_Leaf',petiole,direction,L,W,lmat,rng))
            for sign in [-1,1]:
                d=Vector((math.cos(ta+sign*.4),math.sin(ta+sign*.4),.8))
                L=.067 if kind=='red' else .053
                parts.append(leaf(family+'_TipLeaf',tip,d,L,L*.29,lmat,rng))
    # Harvest sockets have stable IDs and translation-only transforms.
    selected=list(range(len(twigsites)));rng.shuffle(selected);selected=selected[:12 if kind=='red' else 10]
    bushspec={'asset_id':root['asset_id'],'effect_role':effect,'root_height_m':height,'berries':[],'balance_values':None}
    for site_index in selected:
        anchor,tip,a=twigsites[site_index]
        for ci in range(2 if kind=='red' else 3):
            idx=len(bushspec['berries']);stable=family.lower()+'_fruit_'+str(idx+1).zfill(3)
            r=(.014 if kind=='red' else .0115)*rng.uniform(.9,1.12)
            ang=a+(ci-1)*1.35
            attach=anchor+(tip-anchor)*(.65+.10*ci)
            center=attach+Vector((math.cos(ang)*.022,math.sin(ang)*.022,-.032-ci*.007))
            parts.append(branch(family+'_FruitPedicel',[attach,(attach+center)/2+Vector((0,0,.005)),center+Vector((0,0,r))],.00075,.00045))
            sock=bpy.data.objects.new(stable+'_Socket',None);assets.objects.link(sock);sock.parent=root;sock.location=center;sock.empty_display_size=.006
            sock['socket_id']=stable;sock['occupied']=True;sock['berry_effect']=effect;sockets.append(sock)
            ob=fruit(stable,kind,r);ob.parent=sock;ob.location=(0,0,0);ob['asset_id']=stable;ob['game_role']='harvestable_berry';ob['effect_role']=effect;ob['socket_id']=stable
            berries.append(ob)
            bushspec['berries'].append({'id':stable,'node_name':ob.name,'socket_node':sock.name,'position_m_blender_z_up':list(center),'position_m_gltf_y_up':[center.x,center.z,-center.y],'radius_m':r,'effect_role':effect})
    body=join(parts,family+'_Branches_And_Leaves',root);body['game_role']='persistent_bush_body';bodies.append(body);specs.append(bushspec)

def selection(objects):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objects:ob.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
def export(path,objects):
    selection(objects);bpy.ops.export_scene.gltf(filepath=str(OUT/path),export_format='GLB',use_selection=True,export_apply=True,export_yup=True,export_extras=True)

export('berry_bushes_ripe.glb',roots+bodies+sockets+berries)
fruit_samples=[]
for root,body,spec in zip(roots,bodies,specs):
    local_sockets=[s for s in sockets if s.parent==root]
    local_berries=[b for b in berries if b.parent in local_sockets]
    pos=root.location.copy();root.location=(0,0,0);name=spec['asset_id']
    export(name+'_ripe.glb',[root,body]+local_sockets+local_berries)
    root['harvest_state']='harvested'
    for s in local_sockets:s['occupied']=False
    export(name+'_harvested.glb',[root,body]+local_sockets)
    root['harvest_state']='ripe'
    for s in local_sockets:s['occupied']=True
    root.location=pos
    sample=local_berries[0].copy();sample.data=local_berries[0].data.copy();samples.objects.link(sample);sample.parent=None;sample.location=(0,0,0)
    sample.name=spec['effect_role']+'_berry_pickup';sample['asset_id']=sample.name
    if 'socket_id' in sample:del sample['socket_id']
    export(spec['effect_role']+'_berry_pickup.glb',[sample]);sample.hide_render=True;sample.hide_set(True);fruit_samples.append(sample)
(OUT/'harvest_sockets.json').write_text(json.dumps({'units':'meters','bushes':specs,'harvest_contract':'Remove or hide only the berry mesh node; preserve its socket, pedicel and bush body. Regrowth reuses the same socket. Effect amounts, timing and inventory behavior are not implemented.'},indent=2))

def aim(ob,p):ob.rotation_euler=(Vector(p)-ob.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.001));floor=bpy.context.object;move(floor,stage);floor.name='Preview Floor';floor.data.materials.append(plain('Preview Floor',(.045,.055,.06)))
bpy.ops.object.camera_add(location=(1.0,-3.2,1.70));cam=bpy.context.object;move(cam,stage);aim(cam,(0,0,.38));cam.data.type='ORTHO';cam.data.ortho_scale=2.05;scene.camera=cam
for name,loc,power,size in [('Key',(-1,-1.7,2.7),160,2),('Fill',(1.8,-.5,2),110,1.8),('Rim',(0,1.3,2.2),180,1.6)]:
    bpy.ops.object.light_add(type='AREA',location=loc);ob=bpy.context.object;move(ob,stage);ob.name=name;ob.data.energy=power;ob.data.size=size;aim(ob,(0,0,.4))
scene.world.use_nodes=True;scene.world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.30
scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True
scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.view_settings.view_transform='AgX'
selection(bodies)
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':area.spaces.active.region_3d.view_perspective='CAMERA';area.spaces.active.shading.type='MATERIAL'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'berry_bushes.blend'))
scene.render.filepath=str(OUT/'berry_bushes_ripe.png');bpy.ops.render.render(write_still=True)
for b in berries:b.hide_render=True
scene.render.filepath=str(OUT/'berry_bushes_harvested.png');bpy.ops.render.render(write_still=True)
for b in berries:b.hide_render=False

# Inspect actual glTF node metadata and independently reimport the ripe set.
validation={}
for path in sorted(OUT.glob('*.glb')):
    raw=path.read_bytes();n=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+n])
    berry_nodes=[n for n in doc.get('nodes',[]) if n.get('extras',{}).get('game_role')=='harvestable_berry']
    validation[path.name]={'meshes':len(doc.get('meshes',[])),'harvestable_berry_nodes':len(berry_nodes),'embedded_images':all('bufferView' in im for im in doc.get('images',[])),'bytes':len(raw)}
assert validation['berry_bushes_ripe.glb']['harvestable_berry_nodes']==54
assert validation['healing_red_bush_harvested.glb']['harvestable_berry_nodes']==0
assert validation['stamina_blue_bush_harvested.glb']['harvestable_berry_nodes']==0
check=bpy.data.scenes.new('Roundtrip Check');bpy.context.window.scene=check;bpy.ops.import_scene.gltf(filepath=str(OUT/'berry_bushes_ripe.glb'))
validation['roundtrip_mesh_count']=len([o for o in check.objects if o.type=='MESH']);assert validation['roundtrip_mesh_count']==56
manifest={'blender':bpy.app.version_string,'units':'meters','berries':{'healing_red':24,'stamina_blue':30},'mesh_triangle_counts':{},'notes':['Fictional game effects only; no real-world medicinal species claim.','Each berry is an independently removable mesh parented to a stable socket.','Branches, pedicels and leaves remain when berries are removed.','No healing/stamina amounts, harvest logic, regrowth timer, collision or LODs are implemented.']}
for ob in bodies+berries:
    ob.data.calc_loop_triangles();manifest['mesh_triangle_counts'][ob.name]=len(ob.data.loop_triangles)
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2));(OUT/'validation.json').write_text(json.dumps(validation,indent=2))
print('BERRY_BUSH_BUILD_COMPLETE '+json.dumps(validation))
