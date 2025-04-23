/*****************************************************************************/
/*
* PiSHi LE (Lite edition) - Fundamentals of the King's Crook graphics engine.
*
*   by EMMIR 2018-2022
*
*   YouTube: https://www.youtube.com/c/LMP88
*
* This software is released into the public domain.
*/
/*****************************************************************************/

#include "pl.h"

/*  gfx.c
*
* Scans and rasterizes polygons.
*
*/

#include <limits.h>
#include <string.h>
#include <stdlib.h>

int PL_hres;
int PL_vres;
int PL_hres_h;
int PL_vres_h;
int PL_polygon_count;

#if defined(PL_COLOR_DEPTH_32)
int *PL_video_buffer = NULL;
#define PL_VBUFFER_TYPE int*
#elif defined(PL_COLOR_DEPTH_16)
uint16_t *PL_video_buffer = NULL;
#define PL_VBUFFER_TYPE uint16_t*
#elif defined(PL_COLOR_DEPTH_8)
uint8_t *PL_video_buffer = NULL;
#define PL_VBUFFER_TYPE uint8_t*
#endif

#ifdef PL_REDUCED_DEPTH_PRECISION
#define ZBUF_TYPE int16_t
#define ZBUF_SHIFT 16
int16_t *PL_depth_buffer = NULL;
#else
#define ZBUF_TYPE int
#define ZBUF_SHIFT 0
int *PL_depth_buffer = NULL;
#endif

#define ZP 15 /* z precision */

#define TXSH   PL_REQ_TEX_LOG_DIM
#define TXMSK  ((1 << (TXSH + PL_TP)) - 1)
#define TXVMSK (TXMSK & (~((1 << PL_TP) - 1)))

#define ATTRIBS     8
#define ATTRIB_BITS 3
/* y to table */
#define YT(y) ((y) << ATTRIB_BITS)

#define ZL(ytb) ((ytb) + 0)
#define UL(ytb) ((ytb) + 2)
#define VL(ytb) ((ytb) + 4)

#define ZR(ytb) ((ytb) + 1)
#define UR(ytb) ((ytb) + 3)
#define VR(ytb) ((ytb) + 5)

#define SCANP       18
#define SCANP_ROUND (1 << (SCANP - 1))

static int scan_miny;
static int scan_maxy;

/* integer reserve for data locality */
static int g3dresv[PL_MAX_SCREENSIZE               /* x_L */
				+ PL_MAX_SCREENSIZE             /* x_R */
				+ PL_MAX_SCREENSIZE             /* xLc */
				+ PL_MAX_SCREENSIZE             /* xRc */
				+ (ATTRIBS * PL_MAX_SCREENSIZE) /* attrbuf */
];

#define G3R_OFFS_XL   (0)
#define G3R_OFFS_XR   (G3R_OFFS_XL + PL_MAX_SCREENSIZE)
#define G3R_OFFS_XLC  (G3R_OFFS_XR + PL_MAX_SCREENSIZE)
#define G3R_OFFS_XLR  (G3R_OFFS_XLC + PL_MAX_SCREENSIZE)
#define G3R_OFFS_ATTR (G3R_OFFS_XLR + PL_MAX_SCREENSIZE)

static int *x_L = g3dresv + G3R_OFFS_XL;
static int *x_R = g3dresv + G3R_OFFS_XR;
static int *attrbuf = g3dresv + G3R_OFFS_ATTR; /* attribute buffer */

/* cleared versions of scan conversion L/R buffers */
static int *xLc = g3dresv + G3R_OFFS_XLC;
static int *xRc = g3dresv + G3R_OFFS_XLR;

#if defined(PL_PRECALCULATED_MUL8)
#if defined(PL_PRECALCULATED_MUL8_CONST)
#include "mul8_light_table.h"
#else
static unsigned char mul8[256][256];
#endif
#endif

#ifdef PL_REDUCED_DEPTH_PRECISION
#if defined(PL_COLOR_DEPTH_32)
extern void
PL_init(int *video, int16_t *depth, int hres, int vres)
#elif defined(PL_COLOR_DEPTH_16)
extern void
PL_init(uint16_t *video, int16_t *depth, int hres, int vres)
#elif defined(PL_COLOR_DEPTH_8)
extern void
PL_init(uint8_t *video, int16_t *depth, int hres, int vres)
#endif
#else
#if defined(PL_COLOR_DEPTH_32)
extern void
PL_init(int *video, int *depth, int hres, int vres)
#elif defined(PL_COLOR_DEPTH_16)
extern void
PL_init(uint16_t *video, int *depth, int hres, int vres)
#elif defined(PL_COLOR_DEPTH_8)
extern void
PL_init(uint8_t *video, int *depth, int hres, int vres)
#endif
#endif
{
	int i;

	PL_hres = hres;
	PL_vres = vres;
	PL_hres_h = PL_hres >> 1;
	PL_vres_h = PL_vres >> 1;
	PL_set_viewport(0, 0, PL_hres - 1, PL_vres - 1, 1);

	PL_depth_buffer = depth;
	PL_video_buffer = video;

	if (PL_depth_buffer == NULL) {
		EXT_error(PL_ERR_NO_MEM, "gfx", "no depth buffer");
	}

	if (PL_video_buffer == NULL) {
		EXT_error(PL_ERR_NO_MEM, "gfx", "no video buffer");
	}

#if defined(PL_PRECALCULATED_MUL8) && !defined(PL_PRECALCULATED_MUL8_CONST)
	/* 8-bit * 8-bit number multiplication table */
	for (i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			mul8[i][j] = (unsigned char)((i * j) >> 8);
		}
	}
#endif

	/* set buffer offsets */
	x_L = g3dresv + vres;
	x_R = x_L + vres;
	xLc = x_R + vres;
	xRc = xLc + vres;

	for (i = 0; i < vres; i++) {
		xLc[i] = INT_MAX;
		xRc[i] = INT_MIN;
	}

	/* sine is mirrored over X after PI */
	for (i = 0; i < (PL_TRIGMAX >> 1); i++) {
		PL_sin[(PL_TRIGMAX >> 1) + i] = -PL_sin[i];
	}
	/* construct cosine table by copying sine table */
	for (i = 0; i < ((PL_TRIGMAX >> 1) + (PL_TRIGMAX >> 2)); i++) {
		PL_cos[i] = PL_sin[i + (PL_TRIGMAX >> 2)];
	}
	for (i = 0; i < (PL_TRIGMAX >> 2); i++) {
		PL_cos[i + ((PL_TRIGMAX >> 1) + (PL_TRIGMAX >> 2))] = PL_sin[i];
	}
}

#if defined(PL_COLOR_DEPTH_32)
static int
packrgb(int r, int g, int b)
{
	if ((r | g | b) & ~0xff) {
		r = r > 0xff ? 0xff : r;
		g = g > 0xff ? 0xff : g;
		b = b > 0xff ? 0xff : b;
	}
	return (r << 16) | (g << 8) | b;
}

extern void
PL_clear_color_vp(int r, int g, int b)
{
	int hc, x, y, yoff;

	hc = packrgb(r, g, b);

	for (y = PL_vp_min_y; y <= PL_vp_max_y; y++) {
		yoff = y * PL_hres;
		for (x = PL_vp_min_x; x <= PL_vp_max_x; x++) {
			PL_video_buffer[x + yoff] = hc;
		}
	}
}

#elif defined(PL_COLOR_DEPTH_16)
static uint16_t
packrgb(uint8_t r, uint8_t g, uint8_t b)
{
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b & 0xF8 >> 3);
}

extern void
PL_clear_color_vp(uint8_t r, uint8_t g, uint8_t b)
{
	int x, y, yoff;
	uint16_t hc;

	hc = packrgb(r, g, b);

	for (y = PL_vp_min_y; y <= PL_vp_max_y; y++) {
		yoff = y * PL_hres;
		for (x = PL_vp_min_x; x <= PL_vp_max_x; x++) {
			PL_video_buffer[x + yoff] = hc;
		}
	}
}
#elif defined(PL_COLOR_DEPTH_8)
extern void
PL_clear_color_vp(uint8_t r, uint8_t g, uint8_t b)
{
	int x, y, yoff;

	for (y = PL_vp_min_y; y <= PL_vp_max_y; y++) {
		yoff = y * PL_hres;
		for (x = PL_vp_min_x; x <= PL_vp_max_x; x++) {
			PL_video_buffer[x + yoff] = r;
		}
	}
}
#endif

extern void
PL_clear_vp(int r, int g, int b)
{
	PL_clear_color_vp(r, g, b);
	PL_clear_depth_vp();
}

extern void
PL_clear_depth_vp(void)
{
	int x, y, yoff;

	for (y = PL_vp_min_y; y <= PL_vp_max_y; y++) {
		yoff = y * PL_hres;
		for (x = PL_vp_min_x; x <= PL_vp_max_x; x++) {
			PL_depth_buffer[x + yoff] = 0;
		}
	}
}

/* scan convert polygon */
PL_GFX_ATTRIBUTE static int
pscan(int *stream, int dim, int len)
{
	int resv[PL_VDIM + PL_VDIM + (PL_MAX_POLY_VERTS * PL_STREAM_TEX)];

	int rdim;
	int *vA, *vB;
	int x, y, dx, dy;
	int mjr, ady;
	int sx, sy, i;
	int *AS;                        /* attribute buffer ptr */
	int *AT = resv + (0 * PL_VDIM); /* vertex attributes */
	int *DT = resv + (1 * PL_VDIM); /* delta vertex attributes */
	int *VS = resv + (2 * PL_VDIM); /* vertex stream (x-clipped) */
	int *ABL = attrbuf + 0;         /*  left side is +0 */
	int *ABR = attrbuf + 1;         /* right side is +1 */

	rdim = dim - 2;
	scan_miny = INT_MAX;
	scan_maxy = INT_MIN;
	/* clean scan tables */
	memcpy(x_L, xLc, 2 * PL_vres * sizeof(int));

	len = PL_clip_poly_x(VS, stream, dim, len);
	while (len--) {
		vA = VS;
		vB = VS += dim;
		if (!PL_clip_line_y(&vA, &vB, dim, PL_vp_min_y, PL_vp_max_y)) {
			continue;
		}
		x = *vA++;
		y = *vA++;
		dx = *vB++;
		dy = *vB++;
		if (y < scan_miny) {
			scan_miny = y;
		}
		if (y > scan_maxy) {
			scan_maxy = y;
		}
		if (dy < scan_miny) {
			scan_miny = dy;
		}
		if (dy > scan_maxy) {
			scan_maxy = dy;
		}
		dx -= x;
		dy -= y;
		mjr = dx;
		ady = dy;
		if (dx < 0) {
			mjr = -dx;
		}
		if (dy < 0) {
			ady = -dy;
		}
		if (ady > mjr) {
			mjr = ady;
		}
		if (mjr <= 0) {
			continue;
		}
		/* Z precision gets added here */
		AT[0] = vA[0] << ZP;
		DT[0] = ((vB[0] - vA[0]) << ZP) / mjr;
		/* the rest get computed with whatever precision they had */
		for (i = 1; i < rdim; i++) {
			AT[i] = vA[i];
			DT[i] = (vB[i] - vA[i]) / mjr;
		}
		/* make sure to round! */
		x = (x << SCANP) + SCANP_ROUND;
		y = (y << SCANP) + SCANP_ROUND;
		dx = (dx << SCANP) / mjr;
		dy = (dy << SCANP) / mjr;
		do {
			sx = x >> SCANP;
			sy = y >> SCANP;
			if (x_L[sy] > sx) {
				x_L[sy] = sx;
				AS = ABL + YT(sy);
				for (i = 0; i < rdim; i++) {
					AS[i << 1] = AT[i];
				}
			}
			if (x_R[sy] < sx) {
				x_R[sy] = sx;
				AS = ABR + YT(sy);
				for (i = 0; i < rdim; i++) {
					AS[i << 1] = AT[i];
				}
			}
			x += dx;
			y += dy;
			for (i = 0; i < rdim; i++) {
				AT[i] += DT[i];
			}
		} while (mjr--);
	}
	return (scan_miny >= scan_maxy);
}

PL_GFX_ATTRIBUTE extern void
PL_flat_poly(int *stream, int len, int rgb)
{
	int miny, maxy;
	int pos, beg, pbg;
	register PL_VBUFFER_TYPE vbuf;
    ZBUF_TYPE *zbuf;
	int d, yt, dz, sz, dlen;
	int r8 = rgb >> 16 & 0xff;
	int g8 = rgb >> 8 & 0xff;
	int b8 = rgb >> 0 & 0xff;

	if (pscan(stream, PL_STREAM_FLAT, len)) {
		return;
	}
	miny = scan_miny;
	maxy = scan_maxy;
	pos = miny * PL_hres;
	while (miny <= maxy) {
		beg = x_L[miny];
		pbg = pos + beg;
		vbuf = PL_video_buffer + pbg;
		zbuf = PL_depth_buffer + pbg;
		len = x_R[miny] - beg;
		dlen = len + (len == 0);
		yt = YT(miny);
		sz = attrbuf[ZL(yt)];
		dz = (attrbuf[ZR(yt)] - sz) / dlen;

		do {
			if ((*zbuf << ZBUF_SHIFT) < sz) {
                *zbuf = sz >> ZBUF_SHIFT;
				d = (sz >> 20) * 3 / 2;
				if (d >= 256) {
					*vbuf = rgb;
				} else {
					#if defined(PL_PRECALCULATED_MUL8)
					*vbuf = mul8[d][r8] << 16 | mul8[d][g8] << 8 | mul8[d][b8];
					#else
					*vbuf = (d * r8 >> 8) << 16 | (d * g8 >> 8) << 8 | d * b8 >> 8;
					#endif
				}
			}
			sz += dz;
			vbuf++;
			zbuf++;
		} while (len--);
		/* next scanline */
		miny++;
		pos += PL_hres;
	}
	PL_polygon_count++;
}

PL_GFX_ATTRIBUTE extern void
PL_flat_poly_nolight(int *stream, int len, int rgb)
{
    int miny, maxy;
    int pos, beg, pbg;
    register PL_VBUFFER_TYPE vbuf;
    ZBUF_TYPE *zbuf;
    int yt, dz, sz, dlen;

    if (pscan(stream, PL_STREAM_FLAT, len)) {
        return;
    }
    miny = scan_miny;
    maxy = scan_maxy;
    pos = miny * PL_hres;
    while (miny <= maxy) {
        beg  = x_L[miny];
        pbg  = pos + beg;
        vbuf = PL_video_buffer + pbg;
        zbuf = PL_depth_buffer + pbg;
        len  = x_R[miny] - beg;
        dlen = len + (len == 0);
        yt   = YT(miny);
        sz   =  attrbuf[ZL(yt)];
        dz   = (attrbuf[ZR(yt)] - sz) / dlen;

        do {
            if ((*zbuf << ZBUF_SHIFT) < sz) {
                *zbuf = sz >> ZBUF_SHIFT;
                *vbuf = rgb;
            }
            sz += dz;
            vbuf++;
            zbuf++;
        } while (len--);
        /* next scanline */
        miny++;
        pos += PL_hres;
    }
    PL_polygon_count++;
}

PL_GFX_ATTRIBUTE extern void
PL_nodraw_poly(int *stream, int len, int rgb)
{
    int miny, maxy;
    int pos, beg, pbg;
    register PL_VBUFFER_TYPE vbuf;
    ZBUF_TYPE *zbuf;
    int yt, dz, sz, dlen;

    if (pscan(stream, PL_STREAM_FLAT, len)) {
        return;
    }
    miny = scan_miny;
    maxy = scan_maxy;
    pos = miny * PL_hres;
    while (miny <= maxy) {
        beg  = x_L[miny];
        pbg  = pos + beg;
        vbuf = PL_video_buffer + pbg;
        zbuf = PL_depth_buffer + pbg;
        len  = x_R[miny] - beg;
        dlen = len + (len == 0);
        yt   = YT(miny);
        sz   =  attrbuf[ZL(yt)];
        dz   = (attrbuf[ZR(yt)] - sz) / dlen;

        do {
            if ((*zbuf << ZBUF_SHIFT) < sz) {
                *zbuf = sz >> ZBUF_SHIFT;
            }
            sz += dz;
            vbuf++;
            zbuf++;
        } while (len--);
        /* next scanline */
        miny++;
        pos += PL_hres;
    }
    PL_polygon_count++;
}

PL_GFX_ATTRIBUTE extern void
PL_edge_wireframe_poly(int *stream, int len, int rgb)
{
    int miny, maxy;
    int pos, beg, pbg;
    register PL_VBUFFER_TYPE vbuf;
    ZBUF_TYPE *zbuf;
    int yt, dz, sz, dlen;


    if (pscan(stream, PL_STREAM_FLAT, len)) {
        return;
    }
    miny = scan_miny;
    maxy = scan_maxy;
    pos = miny * PL_hres;
    while (miny <= maxy) {
        beg  = x_L[miny];
        pbg  = pos + beg;
        vbuf = PL_video_buffer + pbg;
        zbuf = PL_depth_buffer + pbg;
        len  = x_R[miny] - beg;
        dlen = len + (len == 0);
        yt   = YT(miny);
        sz   =  attrbuf[ZL(yt)];
        dz   = (attrbuf[ZR(yt)] - sz) / dlen;

        do {
            if ((*zbuf << ZBUF_SHIFT) < sz) {
                *zbuf = sz >> ZBUF_SHIFT;
				if(len == 0 || len == x_R[miny] - beg || miny <= scan_miny + 1 || miny >= scan_maxy
- 1)
				{
					*vbuf = rgb;
				}
            }
            sz += dz;
            vbuf++;
            zbuf++;
        } while (len--);
        /* next scanline */
        miny++;
        pos += PL_hres;
    }
    PL_polygon_count++;
}

static void
plot_line (PL_VBUFFER_TYPE vbuf, int color, int x0, int y0, int x1, int y1)
{
	int dx =  abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs (y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2; /* error value e_xy */

	for (;;){  /* loop */
		if (PL_vp_min_x < x0 && PL_vp_max_x > x0 && PL_vp_min_y < y0 && PL_vp_max_y > y0)
			vbuf[x0 + y0 * PL_hres] = color;
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
		if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
	}
}

PL_GFX_ATTRIBUTE extern void
PL_wireframe_poly(int *stream, int len, int rgb)
{
    ZBUF_TYPE *zbuf;

	zbuf = PL_depth_buffer;

	for (int i = 0; i < len * PL_STREAM_FLAT; i += PL_STREAM_FLAT) {
		if (i < (len * PL_STREAM_FLAT - PL_STREAM_FLAT)) {
			plot_line(PL_video_buffer, rgb, stream[i], stream[i+1], stream[i+PL_STREAM_FLAT], stream[i+PL_STREAM_FLAT+1]);
		}
		else {
			plot_line(PL_video_buffer, rgb, stream[i], stream[i+1], stream[0], stream[1]);
		}
	}

    PL_polygon_count++;
}

PL_GFX_ATTRIBUTE extern void
PL_lintx_poly(int *stream, int len, const int *texels)
{
	int miny, maxy;
	int pos, beg, pbg;
	PL_VBUFFER_TYPE vbuf;
    ZBUF_TYPE *zbuf;
	int yt;
	int du = 0, dv = 0, dz;
	int su = 0, sv = 0, sz;
	int r, d, dlen;

	if (pscan(stream, PL_STREAM_TEX, len)) {
		return;
	}
	miny = scan_miny;
	maxy = scan_maxy;
	pos = miny * PL_hres;
	while (miny <= maxy) {
		beg = x_L[miny];
		pbg = pos + beg;
		vbuf = PL_video_buffer + pbg;
		zbuf = PL_depth_buffer + pbg;
		len = x_R[miny] - beg;
		dlen = len + (len == 0);
		yt = YT(miny);
		sz = attrbuf[ZL(yt)];
		dz = (attrbuf[ZR(yt)] - sz) / dlen;
		su = attrbuf[UL(yt)];
		du = (attrbuf[UR(yt)] - su) / dlen;
		sv = attrbuf[VL(yt)];
		dv = (attrbuf[VR(yt)] - sv) / dlen;

		while (len >= 0) {
			if ((*zbuf << ZBUF_SHIFT) < sz) {
                *zbuf = sz >> ZBUF_SHIFT;
				su &= TXMSK;
				sv &= TXMSK;
				/* we can bitwise OR the x and y coordinates together
				* because the texture is guaranteed to be square.
				*/
				yt = texels[(su >> PL_TP) | (sv >> PL_TP << TXSH)];
				d = (sz >> 20) * 3 / 2;
				if (d >= 256) {
					*vbuf = yt;
				} else {
					#if defined(PL_PRECALCULATED_MUL8)
					r = mul8[d][(yt >> 16) & 0xff] << 16;
					r |= mul8[d][(yt >> 8) & 0xff] << 8;
					r |= mul8[d][(yt >> 0) & 0xff] << 0;
					*vbuf = r;
					#else
					/*r  = (d * ((yt >> 16) & 0xff) >> 8) << 16;
					r |= (d * ((yt >> 8) & 0xff) >> 8) <<  8;
					r |= (d * ((yt >> 0) & 0xff) >> 8) <<  0;*/
					*vbuf = ((d * yt) >> 8);
					#endif
				}
			}
			su += du;
			sv += dv;
			sz += dz;
			vbuf++;
			zbuf++;
			len--;
		}
		/* next scanline */
		miny++;
		pos += PL_hres;
	}
	PL_polygon_count++;
}

PL_GFX_ATTRIBUTE extern void
PL_lintx_poly_nolight(int *stream, int len, const int *texels)
{
    int miny, maxy;
    int pos, beg, pbg;
    PL_VBUFFER_TYPE vbuf;
    ZBUF_TYPE *zbuf;
    int yt;
    int du = 0, dv = 0, dz;
    int su = 0, sv = 0, sz;
    int dlen;

    if (pscan(stream, PL_STREAM_TEX, len)) {
        return;
    }
    miny = scan_miny;
    maxy = scan_maxy;
    pos = miny * PL_hres;
    while (miny <= maxy) {
        beg  = x_L[miny];
        pbg  = pos + beg;
        vbuf = PL_video_buffer + pbg;
        zbuf = PL_depth_buffer + pbg;
        len  = x_R[miny] - beg;
        dlen = len + (len == 0);
        yt   = YT(miny);
        sz   =  attrbuf[ZL(yt)];
        dz   = (attrbuf[ZR(yt)] - sz) / dlen;
        su   =  attrbuf[UL(yt)];
        du   = (attrbuf[UR(yt)] - su) / dlen;
        sv   =  attrbuf[VL(yt)];
        dv   = (attrbuf[VR(yt)] - sv) / dlen;

        while (len >= 0) {
            if ((*zbuf << ZBUF_SHIFT) < sz) {
                *zbuf = sz >> ZBUF_SHIFT;
                su &= TXMSK;
                sv &= TXMSK;
                /* we can bitwise OR the x and y coordinates together
                 * because the texture is guaranteed to be square.
                 */
                yt = texels[(su >> PL_TP) | (sv >> PL_TP << TXSH)];
				*vbuf = yt;
            }
            su += du;
            sv += dv;
            sz += dz;
            vbuf++;
            zbuf++;
            len--;
        }
        /* next scanline */
        miny++;
        pos += PL_hres;
    }
    PL_polygon_count++;
}
