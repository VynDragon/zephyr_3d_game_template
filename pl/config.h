#pragma once

#define PL_SIZE_W 256
#define PL_SIZE_H 128

#define MAX(X, Y) ((X>Y)?(X):(Y))

//#define SIZE_WH_MAX MAX(SIZE_W, SIZE_H)

#define PL_SIZE_WH_MAX 256

#define PL_MAX_VERTICES_PER_OBJECT 4096
#define PL_MAX_VERTICES_PER_OBJECT_IMODE 256

/* texture side size is one, shifted by this number must be >= 2 */
#define PL_TEXTURE_SIZE_SHIFT 5 // 5 = 32x32, 4 = 16x16, 6 = 64x64

// 16-bits instead of 32-bits
#define PL_REDUCED_DEPTH_PRECISION

//#define PL_COLOR_DEPTH_32 // default
#define PL_COLOR_DEPTH_8 // L8
//#define PL_COLOR_DEPTH_16 // RGB565

#define PL_GFX_ATTRIBUTE __attribute__((optimize(3)))

// Enable precalculated mul8 for speed, reduces ram space.
#define PL_PRECALCULATED_MUL8
// Enable precalculated mul8 for speed, cancel reduces ram space.
#define PL_PRECALCULATED_MUL8_CONST
