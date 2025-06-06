#include "config.h"
#include "pl.h"
#include "framebuffer_effects.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/timing/timing.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

// intended for 48k dtcm
#if DT_HAS_CHOSEN(zephyr_dtcm)
static __attribute__((section("DTCM"))) uint8_t video_buffer[SIZE_W * PL_SIZE_H];
#else
static uint8_t video_buffer[PL_SIZE_W * PL_SIZE_H];
#endif
static int16_t depth_buffer[PL_SIZE_W * PL_SIZE_H];

static struct PL_OBJ *cube;
static struct PL_OBJ *cube_textured;

static struct PL_TEX checktex;
static int checker[PL_REQ_TEX_DIM * PL_REQ_TEX_DIM];

static void
maketex(void)
{
    int i, j, c;

    for (i = 0; i < PL_REQ_TEX_DIM; i++) {
        for (j = 0; j < PL_REQ_TEX_DIM; j++) {
            if (i < 0x2 || j < 0x2 ||
               (i > ((PL_REQ_TEX_DIM - 1) - 0x2)) ||
               (j > ((PL_REQ_TEX_DIM - 1) - 0x2))) {
                /* border color */
                c = 0xC0;
            } else {
                /* checkered pattern */
                if (((((i & 0x2)) ^ ((j & 0x2))))) {
                    c = 0x0;
                } else {
                    c = 0x50;
                }
            }
            if (i == j || abs(i - j) < 3) {
                /* thick red line along diagonal */
                checker[i + j * PL_REQ_TEX_DIM] = 0xFF;
            } else {
                checker[i + j * PL_REQ_TEX_DIM] = c;
            }
        }
    }
    checktex.data = checker;
}

#include "building_01.h"

static const struct device *display_device = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

int main()
{
	struct display_buffer_descriptor buf_desc;
	int sinvar = 0;
	timing_t start_time, end_time, dstart_time, rend_time, rstart_time;
	uint32_t total_time_us, render_time_us, draw_time_us;
	int close = 800;
	int scroll = 0;
	int close_add = 1;

	if (!device_is_ready(display_device)) {
		printf("Display device not ready");
		return 0;
	}

	timing_init();
	timing_start();

	//k_msleep(1000);
	display_blanking_on(display_device);
	display_set_pixel_format(display_device, PIXEL_FORMAT_L_8);
	display_blanking_off(display_device);

	maketex();

	PL_init(video_buffer, depth_buffer, PL_SIZE_W, PL_SIZE_H);

	//PL_set_viewport(64, 32, 192 - 1, 96 - 1, 1);

	PL_texture(NULL);

	cube = PL_gen_box(32, 32, 32, PL_ALL, 255, 255, 255);

	PL_texture(&checktex);

	cube_textured = PL_gen_box(32, 32, 32, PL_ALL, 255, 255, 255);

	PL_fov = 8;

	PL_cur_tex = NULL;
	PL_cull_mode = PL_CULL_BACK;
	PL_raster_mode = PL_WIREFRAME;

	buf_desc.buf_size = PL_SIZE_W * PL_SIZE_H;
	buf_desc.width = PL_SIZE_W;
	buf_desc.height = PL_SIZE_H;
	buf_desc.pitch = PL_SIZE_W;

	while (1) {
		start_time = timing_counter_get();
		/* clear viewport to black */
		PL_clear_vp(0, 0, 0);
		PL_polygon_count = 0;

		rstart_time = timing_counter_get();
		PL_set_camera(0, 0, 0, 0, 0, 0);

// 		{ /* draw edge cube */
// 			PL_raster_mode = PL_NODRAW;
// 			PL_mst_push();
// 			PL_mst_translate(0, -32, close);
// 			//PL_mst_rotatex(sinvar >> 2);
// 			PL_mst_rotatex(-32);
// 			PL_mst_rotatey(sinvar >> 1);
// 			//PL_mst_scale(PL_P_ONE * ((sinvar & 0xff) + 128) >> 8, PL_P_ONE, PL_P_ONE);
// 			PL_render_object(cube);
// 			PL_mst_pop();
// 		}
// 		simple_edge_2(PL_depth_buffer, PL_video_buffer, PL_SIZE_W * PL_SIZE_H, PL_SIZE_W, 512 << PL_P >>
// 16, 0xFF);
// 		{ /* draw textured cube */
// 			PL_texture(&checktex);
// 			PL_raster_mode = PL_TEXTURED_NOLIGHT;
// 			PL_mst_push();
// 			PL_mst_translate(0, 0, close +200);
// 			//PL_mst_rotatex(sinvar >> 2);
// 			PL_mst_rotatex(32);
// 			//PL_mst_rotatey(sinvar >> 1);
// 			//PL_mst_scale(PL_P_ONE * ((sinvar & 0xff) + 128) >> 8, PL_P_ONE, PL_P_ONE);
// 			PL_render_object(cube_textured);
// 			PL_mst_pop();
// 		}
// 		{ /* draw cube */
// 			PL_raster_mode = PL_FLAT_NOLIGHT;
// 			PL_mst_push();
// 			PL_mst_translate(0, 32, close + 500);
// 			//PL_mst_rotatex(sinvar >> 2);
// 			PL_mst_rotatex(64);
// 			//PL_mst_rotatey(sinvar >> 1);
// 			//PL_mst_scale(PL_P_ONE * ((sinvar & 0xff) + 128) >> 8, PL_P_ONE, PL_P_ONE);
// 			PL_render_object(cube);
// 			PL_mst_pop();
// 		}
// 		{ /* draw wireframe cube */
// 			PL_raster_mode = PL_WIREFRAME;
// 			PL_mst_push();
// 			PL_mst_translate(0, 64, close);
// 			//PL_mst_rotatex(sinvar >> 2);
// 			PL_mst_rotatex(64);
// 			PL_mst_rotatex(sinvar << 1);
// 			//PL_mst_scale(PL_P_ONE * ((sinvar & 0xff) + 128) >> 8, PL_P_ONE, PL_P_ONE);
// 			PL_render_object(cube);
// 			PL_mst_pop();
// 		}
// 		{ /* draw wireframe cube */
// 			PL_raster_mode = PL_FLAT;
// 			PL_mst_push();
// 			PL_mst_translate(-200,0, -400 + scroll);
// 			//PL_mst_rotatex(sinvar >> 2);
// 			PL_mst_rotatex(128);
// 			PL_mst_rotatey(0);
// 			PL_mst_rotatez(64);
// 			//PL_mst_scale(PL_P_ONE * ((sinvar & 0xff) + 128) >> 8, PL_P_ONE, PL_P_ONE);
// 			PL_render_object(&world);
// 			PL_mst_pop();
// 		}
		{ /* draw wireframe cube */
			PL_raster_mode = PL_TEXTURED_NOLIGHT;
			PL_mst_push();
			PL_mst_translate(-10, -100, 1200 + close);
			//PL_mst_rotatex(sinvar >> 2);
			PL_mst_rotatex(sinvar << 1);
			PL_mst_rotatey(128);
			PL_mst_rotatez(64);
			//PL_mst_scale(PL_P_ONE * ((sinvar & 0xff) + 128) >> 8, PL_P_ONE, PL_P_ONE);
			PL_render_object_const(&building_01);
			PL_mst_pop();
		}
		rend_time = timing_counter_get();
		dstart_time = timing_counter_get();
		display_write(display_device, 0, 0, &buf_desc, video_buffer);
		end_time = timing_counter_get();
		total_time_us = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time)) / 1000;
		render_time_us = timing_cycles_to_ns(timing_cycles_get(&rstart_time, &rend_time)) / 1000;
		draw_time_us = timing_cycles_to_ns(timing_cycles_get(&dstart_time, &end_time)) / 1000;
		printf("total us: %u ms:%u fps:%u\n", total_time_us, (total_time_us) / 1000, 1000000 / (total_time_us != 0 ? total_time_us : 1));
		printf("display us:%u render us:%u render fps: %u\n", draw_time_us, render_time_us, 1000000 / (render_time_us != 0 ? render_time_us : 1));
		printf("rendered %u Polygons, %u polygons per second\n", PL_polygon_count, PL_polygon_count * 1000000 / (render_time_us != 0 ? render_time_us : 1));
		sinvar+=1;
		close+=close_add*5;
		if (close > 1000)
		{
			close_add = -1;
		}
		else if (close < -0)
		{
			close_add = 1;
		}
		scroll -= 10;
		k_msleep(1);
		//return 0;
		//msleep(100);
	}

	return 0;
}
