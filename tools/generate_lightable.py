file_out = open("mul8_light_table.h", "wt")

file_out.write("#pragma once\nstatic const unsigned char mul8[256][256] = {\n")

def make_value(i: int, j: int) -> int:
	out = (int(i * 4) * j >> 8) + 16
	return int(255 if out > 255 else (0 if out < 0 else out))

for i in range(0, 256):
	file_out.write("{")
	for j in range(0, 256):
		file_out.write(str(make_value(i,j)) + ", ")
	file_out.write("},\n")
file_out.write("};\n")


