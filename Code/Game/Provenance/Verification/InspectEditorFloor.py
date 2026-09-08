"""Read-only Blender inspection of the authored editor floor in world metres."""
import bpy
from mathutils import Vector

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=r'C:\Users\D-Day\ProvenanceWorkspace\Client\ProvenanceClient\Data\Editor\Floor\floor.fbx')
for obj in bpy.context.scene.objects:
    if obj.type == 'MESH':
        points = [obj.matrix_world @ Vector(v) for v in obj.bound_box]
        print('EDITOR_FLOOR', obj.name, 'min', tuple(min(v[i] for v in points) for i in range(3)),
              'max', tuple(max(v[i] for v in points) for i in range(3)))
