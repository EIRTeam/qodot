#ifndef LIBMAP_BRUSH_H
#define LIBMAP_BRUSH_H

#include <stdlib.h>
#include "vector.h"
#include <vector>
#include <tuple>

struct LMFace;

struct LMBrush {
	int face_count = 0;
	LMFace *faces = NULL;
	vec3 center;
	int vertex_color_count = 0;
	vertexColor *vertex_colors = NULL;
};

#endif