"""Author and export three independently collectible, reference-led stone tools."""
import bpy,bmesh,math,json,random,shutil,sys
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parent;R=random.Random(8314)
refs={'stone_shovel':'C:/Users/D-Day/OneDrive/Pictures/ProvTextures/crude shovel.png','stone_axe':'C:/Users/D-Day/OneDrive/Desktop/ProvRender Prompt/images/Axe-4way HD.png','stone_pickaxe':'C:/Users/D-Day/Downloads/ChatGPT Image Sep 7, 2026, 04_51_22 PM.png'}
for name,src in refs.items():
    d=P/name;d.mkdir(exist_ok=True)
    if Path(src).exists():shutil.copy2(src,d/'reference.png')

def mat(name,rough):
    m=bpy.data.materials.new(name);m.use_nodes=True;b=m.node_tree.nodes['Principled BSDF'];b.inputs['Roughness'].default_value=rough;b.inputs['Metallic'].default_value=0
    for kind,socket in [('color','Base Color'),('normal','Normal')]:
        t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(P/'textures'/(name+'_'+kind+'.png')),check_existing=True);t.image.pack()
        if kind=='normal':
            t.image.colorspace_settings.name='Non-Color';nm=m.node_tree.nodes.new('ShaderNodeNormalMap');m.node_tree.links.new(t.outputs[0],nm.inputs['Color']);m.node_tree.links.new(nm.outputs[0],b.inputs[socket])
        else:m.node_tree.links.new(t.outputs[0],b.inputs[socket])
    return m

def mesh(name,verts,faces,material,uv=None,smooth=False):
    me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update();o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o);me.materials.append(material)
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    if uv:
        layer=me.uv_layers.new()
        for poly in me.polygons:
            for li in poly.loop_indices:layer.data[li].uv=uv[me.loops[li].vertex_index]
    else:
        bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
    for p in me.polygons:p.use_smooth=smooth
    return o

def tube(name,path,radii,material,sides=14):
    verts=[];uv=[];faces=[]
    for i,p in enumerate(path):
        tangent=Vector(path[min(len(path)-1,i+1)])-Vector(path[max(0,i-1)])
        tangent.normalize();u=tangent.cross(Vector((0,1,0))).normalized();w=tangent.cross(u).normalized()
        for j in range(sides+1):
            a=j/sides*math.tau;r=radii[i]*(1+.045*math.sin(j*2.8+i*.45));v=Vector(p)+(u*math.cos(a)+w*math.sin(a))*r
            verts.append(tuple(v));uv.append((j/sides,i/(len(path)-1)))
    for i in range(len(path)-1):
        for j in range(sides):
            a=i*(sides+1)+j;faces.append((a,a+1,a+sides+2,a+sides+1))
    faces.extend([tuple(reversed(range(sides))),tuple((len(path)-1)*(sides+1)+j for j in range(sides))])
    return mesh(name,verts,faces,material,uv,True)

def ribbon(name,points,width,material,thickness=.0009):
    v=[];uv=[]
    for i,p in enumerate(points):
        tangent=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(i-1,0)])
        across=Vector((tangent.z,0,-tangent.x)).normalized()
        for j in [-1,1]:v.append(tuple(Vector(p)+across*width*.5*j));uv.append(((j+1)/2,i/(len(points)-1)))
    o=mesh(name,v,[(2*i,2*i+1,2*i+3,2*i+2) for i in range(len(points)-1)],material,uv)
    mod=o.modifiers.new('Binding thickness','SOLIDIFY');mod.thickness=thickness;bpy.ops.object.modifier_apply(modifier=mod.name)
    return o

def wrap(name,center,radius,z0,z1,turns,width,material):
    v=[];uv=[];steps=int(turns*32)
    for i in range(steps+1):
        t=i/steps;a=t*turns*math.tau;z=z0+(z1-z0)*t
        for edge in [-1,1]:
            zz=z+edge*width*.5;rr=radius(zz);v.append((center(zz)+rr*math.cos(a),rr*.91*math.sin(a),zz));uv.append(((edge+1)/2,t*4))
    o=mesh(name,v,[(2*i,2*i+1,2*i+3,2*i+2) for i in range(steps)],material,uv,True)
    mod=o.modifiers.new('Overlapping hide','SOLIDIFY');mod.thickness=.001;bpy.ops.object.modifier_apply(modifier=mod.name)
    return o

def stone(name,outline,center,thickness,material):
    # Closed radial lens: uneven bevels and percussion facets, thin perimeter.
    n=len(outline);verts=[];faces=[]
    for side in [-1,1]:
        for ring in [1,.82,.50,.15]:
            for k,(x,z) in enumerate(outline):
                xx=center[0]+(x-center[0])*ring;zz=center[1]+(z-center[1])*ring
                half=thickness*(.11 if ring==1 else .73 if ring==.82 else 1 if ring==.5 else .98)
                half*=1+R.uniform(-.10,.10)
                verts.append((xx,side*half,zz))
        for ri in range(3):
            off=(0 if side==-1 else 4*n)+ri*n
            for k in range(n):
                a=off+k;b=off+(k+1)%n;c=b+n;d=a+n
                # Split each annulus into triangles with differing surface slopes.
                faces.extend([(a,b,c),(a,c,d)] if side==1 else [(c,b,a),(d,c,a)])
        off=(0 if side==-1 else 4*n)+3*n
        faces.append(tuple(off+k for k in (range(n) if side==1 else reversed(range(n)))))
    for k in range(n):faces.append((k,(k+1)%n,4*n+(k+1)%n,4*n+k))
    return mesh(name,verts,faces,material)

def lash_head(z,center,extent=.052,depth=.038):
    # All four diagonal straps share the actual bent-haft centreline. The
    # midpoint of each ribbon is exactly above the same head-joint anchor.
    for side in [-1,1]:
        for slope in [-1,1]:
            pts=[]
            for i in range(13):
                t=i/12;zz=z+slope*(t-.5)*.08;x=center(zz)-extent+2*extent*t
                pts.append((x,side*(depth+.001+math.sin(t*math.pi)*.003+(slope+1)*.0015),zz))
            assert abs(pts[6][0]-center(z))<1e-9 and abs(pts[6][2]-z)<1e-9
            ribbon('Crossed rawhide head lashing',pts,.022,fiber)
    # End returns physically connect front and rear straps around the joint.
    for xx,zz in [(-extent,z-.04),(extent,z+.04),(-extent,z+.04),(extent,z-.04)]:
        x=center(zz)+xx
        tube('Rawhide joint return',[(x,depth+.004,zz),(x,0,zz),(x,-depth-.004,zz)],[.007]*3,fiber,8)

def make_axe():
    center=lambda z:.026*math.sin(z/.83*math.pi*1.8)-.015
    radius=lambda z:.020+.011*math.exp(-z*25)+.002*math.sin(z*18)
    joint=.735;mount=center(joint)
    zs=sorted(set([i*.83/36 for i in range(37)]+[joint]));tube('Crooked timber haft',[(center(z),0,z) for z in zs],[radius(z) for z in zs],wood)
    head=stone('Knapped axe head',[(-.207,.814),(-.220,.782),(-.223,.733),(-.213,.680),(-.191,.612),(-.159,.632),(-.102,.670),(-.035,.684),(.048,.690),(.064,.714),(.064,.755),(.045,.779),(-.037,.775),(-.11,.788)],(-.052,.729),.032,rock);head.location.x=mount
    lash_head(joint,center,.04,.034)
    wrap('Neck binding',center,lambda z:radius(z)+.002,.635,.688,5,.012,fiber)
    grip_z=[.095+i*.210/16 for i in range(17)];tube('Hide grip underlay',[(center(z),0,z) for z in grip_z],[radius(z)+.001 for z in grip_z],leather)
    wrap('Leather hand grip',center,lambda z:radius(z)+.002,.11,.29,7,.029,leather)
    wrap('Grip retaining cord',center,lambda z:radius(z)+.003,.10,.115,2,.005,fiber)
    ribbon('Loose hide grip tail',[(center(.11)+.018,-.011,.11),(.026,-.016,.074),(.033,-.004,.054)],.014,leather)
    tube('Wooden locking wedge',[(center(z),-.026,z) for z in [.685,.735,.775,.810]],[.010,.011,.012,.012],wood,10)
    # Small green decorative peg from the multiview, interpreted as a stone inset.
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,radius=.009,location=(center(.797),-.043,.797));o=bpy.context.object;o.scale=(1,.36,1);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(accent)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project();bpy.ops.object.mode_set(mode='OBJECT')
    return {'grip':[center(.20),0,.20],'head_joint':[mount,0,joint],'lashing_cross_front':[mount,-.0395,joint],'primary_strike':[-.218+mount,0,.719]},.83

def make_pick():
    center=lambda z:.037*math.sin(z/.98*math.pi*1.75)-.012
    radius=lambda z:.022+.012*math.exp(-z*22)
    joint=.891;mount=center(joint)
    zs=sorted(set([i*.98/40 for i in range(41)]+[joint]));tube('Crooked timber haft',[(center(z),0,z) for z in zs],[radius(z) for z in zs],wood)
    head=stone('Point and adze stone head',[(-.345,.797),(-.303,.841),(-.251,.880),(-.182,.918),(-.09,.94),(-.015,.942),(.080,.93),(.175,.928),(.298,.920),(.301,.875),(.291,.791),(.232,.821),(.176,.850),(.081,.870),(-.023,.865),(-.132,.870),(-.223,.845),(-.290,.819)],(-.005,.899),.036,rock);head.location.x=mount
    lash_head(joint,center,.045,.038)
    wrap('Rawhide neck collar',center,lambda z:radius(z)+.004,.775,.827,5,.014,fiber)
    grip_z=[.100+i*.237/18 for i in range(19)];tube('Hide grip underlay',[(center(z),0,z) for z in grip_z],[radius(z)+.001 for z in grip_z],leather)
    wrap('Long hide grip',center,lambda z:radius(z)+.002,.115,.322,8,.029,leather)
    wrap('Grip tie',center,lambda z:radius(z)+.004,.103,.116,2,.005,fiber)
    ribbon('Loose grip tail',[(center(.11)+.02,-.02,.112),(.031,-.015,.079),(.042,-.005,.048)],.016,leather)
    tube('Timber locking wedge',[(center(z),-.026,z) for z in [.821,.865,.891,.928,.956]],[.011,.012,.013,.013,.013],wood,10)
    return {'grip':[center(.21),0,.21],'head_joint':[mount,0,joint],'lashing_cross_front':[mount,-.0435,joint],'primary_strike':[-.345+mount,0,.797],'secondary_strike':[.297+mount,0,.85]},.98

def make_shovel():
    stone('Broad chipped stone spade',[(-.132,.313),(-.137,.287),(-.135,.223),(-.122,.161),(-.087,.092),(-.041,.031),(0,0),(.046,.032),(.089,.092),(.122,.16),(.137,.228),(.134,.290),(.124,.313),(.06,.311),(0,.308),(-.06,.311)],(0,.19),.024,rock)
    zs=[.29+i*.665/28 for i in range(29)];tube('Timber shaft and blade socket',[(.004*math.sin(z*8),0,z) for z in zs],[.024 if z<.4 else .020 for z in zs],wood)
    tube('Blade reinforcing timber tongue',[(0,-.025,.145),(0,-.032,.24),(0,-.012,.38)],[.002,.018,.024],wood,10)
    # Timber fork and crossbar form an open D handle, not a filled silhouette.
    for sign in [-1,1]:
        tube('Bent D handle fork',[(sign*.012,0,.937),(sign*.041,0,.994),(sign*.075,0,1.035),(sign*.095,0,1.095),(sign*.098,0,1.166)],[.016,.013,.012,.013,.016],wood,12)
    tube('D handle cross grip',[(-.102,0,1.165),(-.060,0,1.167),(0,0,1.164),(.06,0,1.165),(.102,0,1.165)],[.023,.024,.025,.024,.023],wood)
    wrap('Upper shaft rawhide',lambda z:.004*math.sin(z*8),lambda z:.023,.879,.961,6,.018,leather)
    wrap('Blade socket rawhide',lambda z:.004*math.sin(z*8),lambda z:.027,.306,.411,7,.019,leather)
    # Broad bindings pin the stone to the timber tongue instead of a metal rivet.
    for side in [-1,1]:
        ribbon('Stone socket diagonal tie',[(-.040,side*.025,.262),(0,side*.034,.3),(.036,side*.028,.337)],.012,fiber)
    return {'grip':[0,0,1.165],'support_hand':[0,0,.65],'primary_strike':[0,0,0]},1.19

def aim(o,point):o.rotation_euler=(Vector(point)-o.location).to_track_quat('-Z','Y').to_euler()
def export(file,objects):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.export_scene.gltf(filepath=str(file),export_format='GLB',use_selection=True,export_apply=True,export_yup=True,export_extras=True)

def clean(o):
    bm=bmesh.new();bm.from_mesh(o.data)
    bmesh.ops.triangulate(bm,faces=list(bm.faces))
    dead=[f for f in bm.faces if f.calc_area()<1e-13]
    if dead:bmesh.ops.delete(bm,geom=dead,context='FACES')
    bm.to_mesh(o.data);bm.free();o.data.update()

reports=[]
for asset,make in [('stone_shovel',make_shovel),('stone_axe',make_axe),('stone_pickaxe',make_pick)]:
    bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene;scene.unit_settings.system='METRIC'
    rock=mat('stone',.84);wood=mat('wood',.68);fiber=mat('fiber',.87);leather=mat('leather',.60);accent=mat('accent',.52)
    anchors,height=make();parts=[]
    for m in [rock,wood,fiber,leather,accent]:
        obs=[o for o in list(scene.objects) if o.type=='MESH' and o.data.materials and o.data.materials[0]==m]
        if not obs:continue
        bpy.ops.object.select_all(action='DESELECT')
        for o in obs:o.select_set(True)
        bpy.context.view_layer.objects.active=obs[0];bpy.ops.object.join();o=bpy.context.object;o.name=asset+'_'+m.name;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);clean(o);parts.append(o)
    dest=P/asset;export(dest/(asset+'_lod0.glb'),parts)
    lod=[]
    for o in parts:
        cp=o.copy();cp.data=o.data.copy();scene.collection.objects.link(cp);bpy.context.view_layer.objects.active=cp
        mod=cp.modifiers.new('Distance detail','DECIMATE');mod.ratio=.52;bpy.ops.object.modifier_apply(modifier=mod.name);clean(cp);lod.append(cp)
    export(dest/(asset+'_lod1.glb'),lod)
    triangles=lambda obs:sum((o.data.calc_loop_triangles() or len(o.data.loop_triangles)) for o in obs)
    counts=[triangles(parts),triangles(lod)]
    for o in lod:bpy.data.objects.remove(o,do_unlink=True)
    pts=[o.matrix_world@v.co for o in parts for v in o.data.vertices];bounds=[[min(p[k] for p in pts) for k in range(3)],[max(p[k] for p in pts) for k in range(3)]]
    for name,pos in anchors.items():
        e=bpy.data.objects.new(name,None);scene.collection.objects.link(e);e.location=pos;e.empty_display_size=.015;e['role']=name
    bpy.ops.wm.save_as_mainfile(filepath=str(dest/(asset+'.blend')))
    bpy.ops.object.camera_add(location=(-height*.72,-height*2,height*.94));cam=bpy.context.object;aim(cam,(0,0,height*.49));cam.data.type='ORTHO';cam.data.ortho_scale=height*1.24;scene.camera=cam
    for loc,power,size in [((-.8,-1.3,1.8),95,1),((.9,-.6,1.1),40,.9),((0,1,1.6),90,.8)]:
        bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.size=size;aim(o,(0,0,height*.5))
    world=bpy.data.worlds.new('Studio');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[1].default_value=.18;scene.world=world
    scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True;scene.render.film_transparent=True;scene.render.image_settings.color_mode='RGBA';scene.view_settings.view_transform='AgX'
    if '--skip-render' not in sys.argv:
        scene.render.resolution_x=900;scene.render.resolution_y=1200;scene.render.resolution_percentage=100;scene.render.filepath=str(dest/'review.png');bpy.ops.render.render(write_still=True)
        scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.filepath=str(dest/'inventory_icon.png');bpy.ops.render.render(write_still=True)
        if asset!='stone_shovel':
            for name,loc in [('front',(0,-3,height*.5)),('right',(3,0,height*.5))]:
                cam.location=loc;aim(cam,(0,0,height*.5));cam.data.ortho_scale=height*1.12
                scene.render.resolution_x=850;scene.render.resolution_y=1100;scene.render.filepath=str(dest/(name+'_alignment.png'));bpy.ops.render.render(write_still=True)
    report={'id':asset,'units':'metres','blender_and_engine_up':'Z','gltf_up':'Y','bounds_z_up':bounds,'anchors_z_up':anchors,'visual_lods':[asset+'_lod0.glb',asset+'_lod1.glb'],'triangles':counts,'materials':[o.data.materials[0].name for o in parts],'interpretation':'Reference-led silhouette and bindings. Hidden construction inferred; shovel blade interpreted as stone per user designation, with timber tongue and hide attachment.'}
    (dest/'manifest.json').write_text(json.dumps(report,indent=2));reports.append(report);print('TOOL_COMPLETE',asset,counts,flush=True)
(P/'manifest.json').write_text(json.dumps({'assets':reports,'pickup':'Separate E-key pickups, current Play-session ownership.'},indent=2))
