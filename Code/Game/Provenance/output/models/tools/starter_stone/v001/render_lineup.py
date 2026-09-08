import bpy,math
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene
for name,x,label in [('stone_shovel',-.73,'Stone shovel'),('stone_axe',0,'Stone axe'),('stone_pickaxe',.73,'Stone pickaxe')]:
    before=set(scene.objects);bpy.ops.import_scene.gltf(filepath=str(P/name/(name+'_lod0.glb')))
    for o in set(scene.objects)-before:
        if o.parent is None:o.location.x+=x
    bpy.ops.object.text_add(location=(x,-.10,-.070),rotation=(math.pi/2,0,0));o=bpy.context.object;o.data.body=label;o.data.align_x='CENTER';o.data.size=.040
    m=bpy.data.materials.get('Label') or bpy.data.materials.new('Label');m.diffuse_color=(.75,.75,.75,1);o.data.materials.append(m)
def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(0,-4,.58));cam=bpy.context.object;aim(cam,(0,0,.58));cam.data.type='ORTHO';cam.data.ortho_scale=2.24;scene.camera=cam
for loc,power,size in [((-1,-1.5,2.5),190,2),((1.5,-1,1.8),105,1.5),((0,1,2),190,1.5)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.size=size;aim(o,(0,0,.6))
w=bpy.data.worlds.new('Studio');w.use_nodes=True;w.node_tree.nodes['Background'].inputs[1].default_value=.20;scene.world=w
scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True;scene.render.film_transparent=True;scene.render.image_settings.color_mode='RGBA';scene.view_settings.view_transform='AgX'
scene.render.resolution_x=1800;scene.render.resolution_y=1150;scene.render.resolution_percentage=100;scene.render.filepath=str(P/'starter_tools_lineup.png');bpy.ops.render.render(write_still=True)
