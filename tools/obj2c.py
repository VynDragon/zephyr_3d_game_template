import pywavefront
import argparse
from pathlib import Path
from PIL import Image

parser = argparse.ArgumentParser(
                    prog='obj2c',
                    description='convert obj file to PL3D-KC object structure')

parser.add_argument('filename_in')
parser.add_argument('filename_out')

args = parser.parse_args()

object_in = pywavefront.Wavefront(args.filename_in, collect_faces=True)
file_out = open(args.filename_out, "xt")

data_name = Path(args.filename_in).stem
data_vertices_name = data_name + "_vertices"
data_polys_name = data_name + "_polys"

MUL_FLOAT = 100

file_out.write("#pragma once\nstatic const int " +  data_vertices_name + "[] = {\n")

for count, vertex in enumerate(object_in.vertices):
	x = int(vertex[0] * MUL_FLOAT)
	y = int(vertex[1] * MUL_FLOAT)
	z = int(vertex[2] * MUL_FLOAT)
	w = 0
	file_out.write(str(x) + "," + str(y) + "," + str(z) + "," + str(w) + ",\n")

file_out.write("};\n")

file_out.write("static const struct PL_POLY " +  data_polys_name + "[] = {\n")

total_polys = 0

for mesh in object_in.mesh_list:
	total_polys = total_polys + len(mesh.faces)
	for face in mesh.faces:
		file_out.write("{ ")
		file_out.write(".tex = NULL, ")
		file_out.write(".n_verts = 3, .verts = ")
		if len(mesh.materials) > 0:
			file_out.write("{ " + str(face[0]) + ", 0, 0, " + str(face[1]) + ", 0, 0, " + str(face[2]) + ", 0, 0, " + str(face[0]) + ", 0, 0}, ")
			file_out.write(".color = " + str(int(((mesh.materials[0].diffuse[0] + mesh.materials[0].diffuse[1] + mesh.materials[0].diffuse[2]) / 3) * 256)) + "},\n")
		else:
			file_out.write("{ " + str(face[0]) + ", 0, 0, " + str(face[1]) + ", 0, 0, " + str(face[2]) + ", 0, 0, " + str(face[0]) + ", 0, 0}, ")
			file_out.write(".color = " + str(int(0xFF)) + "},\n")
		#for key, value in object_in.materials.items():
		#	if face[0] in value.vertices or face[1] in value.vertices or face[2] in value.vertices:
		#		file_out.write(".color = " + str(int(((value.diffuse[0] + value.diffuse[1] + value.diffuse[2]) / 3) * 256)) + "},\n")


file_out.write("};\n")

file_out.write("static const struct PL_OBJ " +  data_name + " = {\n")
file_out.write(".verts = " + data_vertices_name + ",\n")

file_out.write(".n_polys = " + str(int(total_polys)) + ",\n")
file_out.write(".n_verts = " + str(len(object_in.vertices)) + ",\n")
file_out.write(".polys = " + data_polys_name + ",\n")
file_out.write("};\n")
