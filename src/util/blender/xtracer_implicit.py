bl_info = {
	"name" : "Xtracer / Implicit Geometry",
	"author": "Nikos Papadopoulos",
	"version": (1, 0, 0),
	"blender": (2, 72, 0),
	"location": "Add > Mesh",
	"description": "Create implicit geometry.",
	"warning": "",
	"wiki_url": "",
	"tracker_url": "",
	"category": "Add Mesh"
}


import bpy
import bmesh
import math
from mathutils import *

class OBJECT_OT_implicit_sphere(bpy.types.Operator):
	bl_idname = "mesh.implicit_sphere"
	bl_label = "Add Implicit Sphere"
	bl_options = {'REGISTER', 'UNDO'}
	bl_description = "Add an implicit sphere"
	def execute(self, context):
		return{'FINISHED'}

class OBJECT_OT_implicit_plane(bpy.types.Operator):
	bl_idname = "mesh.implicit_plane"
	bl_label = "Add Implicit Plane"
	bl_options = {'REGISTER', 'UNDO'}
	bl_description = "Add a implicit plane" #tooltip
	def execute(self, context):
		return{'FINISHED'}

def menu_item(self, context):
	self.layout.operator(OBJECT_OT_implicit_sphere.bl_idname, text="Implicit Sphere", icon="PLUGIN")
	self.layout.operator(OBJECT_OT_implicit_plane.bl_idname, text="Implicit Plane", icon="PLUGIN")


def register():
	bpy.utils.register_module(__name__)
	bpy.types.INFO_MT_add.append(menu_item)


def unregister():
	bpy.utils.unregister_module(__name__)
	bpy.types.INFO_MT_add.remove(menu_item)


if __name__ == "__main__":
	register()
