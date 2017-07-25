import bpy
from mathutils import Vector

bl_info = {
	"name"			: "xtracer",
	"description" 	: "An open-source ray tracer project",
	"author"	  	: "Nikos Papadopoulos",
	"category"		: "Render",
	"warning"	 	: "Still under development"
}

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

def export(filename):
	print(filename)

	with open(filename, 'w') as f:
		f.write("# Generated using Blender\n")
		f.write("title = \n")
		f.write("descr = \n")
		f.write("versn = \n")
		f.write("\n")

		f.write("camera = {\n")
		for ob in bpy.context.scene.objects:
			if ob.type == 'CAMERA':
				print("exporting camera ", ob.name)
				f.write("\t" + ob.name + " = {\n")
				p = ob.matrix_world.to_translation()
				u = ob.matrix_world.to_quaternion() * Vector((0.0, 1.0, 0.0))
				d = ob.matrix_world.to_quaternion() * Vector((0.0, 0.0,-1.0))
				f.write("\t\ttype     = thin-lens\n")
				f.write("\t\tfov      = 45\n")
				f.write("\t\tposition = vec3(" + str(p.x) + ", " + str(p.y) + ", " + str(p.z) + ")\n")
				f.write("\t\ttarget   = vec3(" + str(d.x) + ", " + str(d.y) + ", " + str(d.z) + ")\n")
				f.write("\t\tup       = vec3(" + str(u.x) + ", " + str(u.y) + ", " + str(u.z) + ")\n")

				print(ob.location)
				f.write("\t}\n")
		f.write("}\n")

		f.write("geometry = {\n")
		for ob in bpy.context.scene.objects:
			if ob.type == 'MESH':
				f.write("\t" + ob.name + " = {\n")
				print("exporting mesh   ", ob.name)
				f.write("\t}\n")
		f.write("}\n")

	return {'FINISHED'}

def menu_func(self, context):
	self.layout.operator(xtracer.bl_idname, text="XTracer (.scn)")

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
