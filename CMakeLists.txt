cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(3d_game_template)

target_sources(app PRIVATE src/main.c)

zephyr_syscall_include_directories(include)

zephyr_include_directories(include)
zephyr_include_directories(pl)


#PL3D-KC
target_sources(app PRIVATE
pl/clip.c
pl/gfx.c
pl/imode.c
pl/math.c
pl/pl.c
pl/framebuffer_effects.c
pl/hal.c
)
