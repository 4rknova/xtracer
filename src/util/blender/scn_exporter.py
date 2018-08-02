import math
import bpy
from mathutils import Vector

bl_info = {
	"name"			: "Xtracer (*.scn)",
	"author"		: "Nikos Papadopoulos",
	"version"		: (0,1,0),
	"blender"		: (2,7,6),
	"email"			: "nikpapas@gmail.com",
	"location"		: "File > Export > xtracer (*.scn)",
	"description" 	: "Scene file exporter for xtracer",
	"category"		: "Import-Export",
	"warning"	 	: "Still under development",
	"wiki_url"		: "https://github.com/4rknova/xtracer/wiki",
	"tracker_url"   : "https://github.com/4rknova/xtracer/issues"
}

__version__ = bl_info["version"]
__url__		= bl_info["wiki_url"]
__email__   = bl_info["email"]
__author__  = bl_info["author"]
__bpydoc__  = bl_info["description"]
__version__ = bl_info["version"]

class entity():
	def __init__(self):
		self.name = ""
		self.properties = []

class scene():
	def __init__(self):
		self.cameras   = []
		self.geometry  = []
		self.materials = []
		self.objects   = []

	def parse(self):
		for ob in bpy.context.scene.objects:
			if ob.type == 'CAMERA':
				p = ob.matrix_world.to_translation()
				u = ob.matrix_world.to_quaternion() * Vector((0.0, 1.0, 0.0))
				d = ob.matrix_world.to_quaternion() * Vector((0.0, 0.0, 1.0))
				cam = entity();
				cam.name = ob.name
				cam.properties.append(("type", "thin-lens"))
				cam.properties.append(("fov", math.degrees(ob.data.angle_x)))
				cam.properties.append(("position", "vec3(" + str(p.x) + ", " + str(p.y) + ", " + str(p.z) + ")"))
				cam.properties.append(("target", "vec3(" + str(d.x) + ", " + str(d.y) + ", " + str(d.z) + ")"))
				cam.properties.append(("up", "vec3(" + str(u.x) + ", " + str(u.y) + ", " + str(u.z) + ")"))
				self.cameras.append(cam)

#				mat = entity()
#				slot = obj.material_slots[f.material_index]
#				mat = slot.material


#		f.write("geometry = {\n")
#		for ob in bpy.context.scene.objects:
#			if ob.type == 'MESH':
#				print("exporting mesh   ", ob.name)
#				#f.write("\t" + ob.name + " = {\n")
#				#f.write("\t}\n")
			if ob.type == 'MESH':
				p = ob.matrix_world.to_translation()
				if "implicit_type" in ob.keys():
					if ob["implicit_type"] == 1.0: # Sphere
						geo = entity()
						geo.name = ob.name
						geo.properties.append(("type", "sphere"))
						geo.properties.append(("position", "vec3(" + str(p.x) + ", " + str(p.y) + ", " + str(p.z) + ")"))
						geo.properties.append(("radius", str(ob.empty_draw_size)))
						self.geometry.append(geo)

	def serialize(self, filename):
		with open(filename, 'w') as f:
			f.write("# Generated using Blender\n")
			f.write("title = " + bpy.context.scene.name + "\n")
			f.write("descr = \n")
			f.write("versn = \n")
			f.write("\n")
			f.write("environment = {\n")
			f.write("	type = gradient\n")
			f.write("	config = {\n")
			f.write("		a = col3(1.0,1.0,1.0)\n")
			f.write("		b = col3(0.7,0.8,1.0)\n")
			f.write("	}\n")
			f.write("}\n")
			f.write("\n")
			f.write("camera = {\n")
			for cam in self.cameras:
				serialize(f, cam)
			f.write("}\n")
			f.write("geometry = {\n")
			for geo in self.geometry:
				serialize(f, geo)
			f.write("}\n")
			f.write("material = {\n")
			for mat in self.materials:
				serialize(f, mat)
			f.write("}\n")
			f.write("object = {\n")
			for obj in self.objects:
				serialize(f, obj)
			f.write("}\n")

class xtracer(bpy.types.Operator) :
	bl_idname	 = "scene.xtracer"
	bl_label	 = "Export scene"
	bl_options	 = {'PRESET'}
	filepath	 = bpy.props.StringProperty(subtype="FILE_PATH")
	filename_ext = ".scn"

	def invoke(self, context, event):
		context.window_manager.fileselect_add(self)
		return {'RUNNING_MODAL'}

	def execute(self, context):
		return export(self.filepath)

def serialize(stream, item):
	stream.write("\t" + item.name + " = {\n")
	for entry in item.properties:
		stream.write("\t\t" + str(entry[0]) + " = " + str(entry[1]) + "\n")
	stream.write("\t}\n")

def export(filename):
	s = scene()
	s.parse()
	s.serialize(filename)
	return {'FINISHED'}

def menu_func(self, context):
	self.layout.operator(xtracer.bl_idname, text="Xtracer (.scn)")

def register():
	print("Registering..")
	bpy.utils.register_module(__name__)
	bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
	print("Unregistering..")
	bpy.utils.unregister_module(__name__)
	bpy.types.INFO_MT_file_export.remove(menu_func)


# This allows you to run the script directly from blender's text editor
# to test the addon without having to install it.
if __name__ == "__main__":
	register()
