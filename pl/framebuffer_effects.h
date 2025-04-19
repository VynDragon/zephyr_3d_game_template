#pragma once

#include <stdint.h>

void simple_edge(int *in, int *out, int len, int delta, int edgeColor);

#if defined(PL_COLOR_DEPTH_32)
void simple_edge_2(int16_t *in, int *out, int len, int width, int delta, int edgeColor);
#elif defined(PL_COLOR_DEPTH_16)
void simple_edge_2(int16_t *in, uint16_t *out, int len, int width, int delta, uint16_t edgeColor);
#elif defined(PL_COLOR_DEPTH_8)
void simple_edge_2(int16_t *in, uint8_t *out, int len, int width, int delta, uint8_t edgeColor);
#endif
