#include "config.h"
#include <stdint.h>

int delta_eq(int a, int b, int delta)
{
	if (a > b - delta && a < b + delta) return 1;
	return 0;
}

void simple_edge(int *in, int *out, int len, int delta, int edgeColor)
{
	for(int i = 1; i < len; i++)
	{
		if (delta_eq(in[i], in[i - 1], delta) == 0)
		{
			out[i] = edgeColor;
		}
	}
}

#if defined(PL_COLOR_DEPTH_32)
void simple_edge_2(int16_t *in, int *out, int len, int width, int delta, int edgeColor)
#elif defined(PL_COLOR_DEPTH_16)
void simple_edge_2(int16_t *in, uint16_t *out, int len, int width, int delta, uint16_t edgeColor)
#elif defined(PL_COLOR_DEPTH_8)
void simple_edge_2(int16_t *in, uint8_t *out, int len, int width, int delta, uint8_t edgeColor)
#endif
{
	for(int i = 1; i < len - width; i++)
	{
		if (delta_eq(in[i], in[i - 1], delta) == 0 || delta_eq(in[i], in[i + width], delta) == 0 )
		{
			out[i] = edgeColor;
		}
	}
}




