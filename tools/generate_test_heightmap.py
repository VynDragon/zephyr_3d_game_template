file_out = open("demo_terrain.h", "wt")

file_out.write("#pragma once\nstatic const uint8_t terrain_color[64*64] = {\n")

for x in range(0,32):
	for j in range(0, 16):
		file_out.write(str(255-64+j*4) + ", ")
	for j in range(0, 16):
		file_out.write(str(255-j*4) + ", ")
	for j in range(0, 16):
		file_out.write(str(191-j*4) + ", ")
	for j in range(0, 16):
		file_out.write(str(191-64+j*4) + ", ")
for x in range(0,32):
	for j in range(0, 32):
		file_out.write(str(127) + ", ")
	for j in range(0, 32):
		file_out.write(str(8) + ", ")
file_out.write("};\n")

file_out.write("\n\nstatic const int terrain_height[64*64] = {\n")

for x in range(0,32):
	for j in range(0, 16):
		file_out.write(str(1024-64+j*4) + ", ")
	for j in range(0, 16):
		file_out.write(str(1024-j*4) + ", ")
	for j in range(0, 16):
		file_out.write(str(512-j*4) + ", ")
	for j in range(0, 16):
		file_out.write(str(512-64+j*4) + ", ")
for x in range(0,32):
	for j in range(0, 32):
		file_out.write(str(256) + ", ")
	for j in range(0, 32):
		file_out.write(str(64) + ", ")
file_out.write("};\n")
