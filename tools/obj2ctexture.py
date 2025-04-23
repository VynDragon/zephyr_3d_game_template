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
TEXSQUARE_SIZE = 32

file_out.write("#pragma once\nstatic const int " +  data_vertices_name + "[] = {\n")

for count, vertex in enumerate(object_in.vertices):
	x = -int(vertex[0] * MUL_FLOAT)
	y = -int(vertex[1] * MUL_FLOAT)
	z = -int(vertex[2] * MUL_FLOAT)
	w = 0
	file_out.write(str(x) + "," + str(y) + "," + str(z) + "," + str(w) + ",\n")

file_out.write("};\n")

for name, value in object_in.materials.items():
	if value.texture:
		tex = Image.open(value.texture.find())
		tex = tex.resize((TEXSQUARE_SIZE, TEXSQUARE_SIZE))
		tex = tex.convert("L")
		file_out.write("static const int " +  data_name + name + "texturedata[" + str(TEXSQUARE_SIZE * TEXSQUARE_SIZE) + "] = {\n")
		for p in list(tex.getdata()):
			file_out.write(str(p) + ",")
		file_out.write("};\n")
		file_out.write("static const struct PL_TEX_CONST " +  data_name + name + "texture = {\n")
		file_out.write(".data = " + data_name + name + "texturedata ,\n")
		file_out.write("};\n")


file_out.write("static const struct PL_POLY_CONST " +  data_polys_name + "[] = {\n")

total_polys = 0

def get_uv(idx, material):
	sequence = list(object_in.vertices[idx])
	indexes = []
	for i in range(len(material.vertices)):
		if material.vertices[i:i+len(sequence)] == sequence:
			indexes.append((i, i+len(sequence)))
	print(indexes)
	return (material.vertices[indexes[0][0] - 2], material.vertices[indexes[0][0] - 2])

for mesh in object_in.mesh_list:
	total_polys = total_polys + len(mesh.faces)
	for faceid, face in enumerate(mesh.faces):
		file_out.write("{ ")
		file_out.write(".tex = &")
		file_out.write(str(data_name) + mesh.materials[0].name + "texture")
		file_out.write(", ")
		file_out.write(".n_verts = 3, .verts = ")
		#face0uv = get_uv(face[0], mesh.materials[0])
		#face1uv = get_uv(face[1], mesh.materials[0])
		#face2uv = get_uv(face[2], mesh.materials[0])
		face0uv = (mesh.materials[0].vertices[faceid * 5 * 3], mesh.materials[0].vertices[faceid * 5 * 3+ 1])
		face1uv = (mesh.materials[0].vertices[faceid * 5 * 3 + 5], mesh.materials[0].vertices[faceid * 5 * 3 + 6])
		face2uv = (mesh.materials[0].vertices[faceid * 5 * 3 + 10], mesh.materials[0].vertices[faceid * 5 * 3 + 11])

		file_out.write("{ " + str(face[0]) + ", " + str(int(face0uv[0] * TEXSQUARE_SIZE)) + ", " + str(int(face0uv[1] * TEXSQUARE_SIZE)))
		file_out.write(", " + str(face[1]) + ", " + str(int(face1uv[0] * TEXSQUARE_SIZE)) + ", " + str(int(face1uv[1] * TEXSQUARE_SIZE)))
		file_out.write(", " + str(face[2]) + ", " + str(int(face2uv[0] * TEXSQUARE_SIZE)) + ", " + str(int(face2uv[1] * TEXSQUARE_SIZE)))
		file_out.write(", " + str(face[0]) + ", " + str(int(face0uv[0] * TEXSQUARE_SIZE)) + ", " + str(int(face0uv[1] * TEXSQUARE_SIZE)))
		file_out.write("},\n")
		file_out.write(".color = " + str(int(0xFF)) + "},\n")
		#for key, value in object_in.materials.items():
		#	if face[0] in value.vertices or face[1] in value.vertices or face[2] in value.vertices:
		#		file_out.write(".color = " + str(int(((value.diffuse[0] + value.diffuse[1] + value.diffuse[2]) / 3) * 256)) + "},\n")


file_out.write("};\n")

file_out.write("static const struct PL_OBJ_CONST " +  data_name + " = {\n")
file_out.write(".verts = " + data_vertices_name + ",\n")

file_out.write(".n_polys = " + str(int(total_polys)) + ",\n")
file_out.write(".n_verts = " + str(len(object_in.vertices)) + ",\n")
file_out.write(".polys = " + data_polys_name + ",\n")
file_out.write("};\n")
