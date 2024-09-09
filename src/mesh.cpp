#include "mesh.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sys_utils.h"
#include "vec2.h"
#include "vec3.h"

void MBuf::clear()
{
	MEMFREE(indices);
	idx_capacity = 0;
	MEMFREE(positions);
	MEMFREE(normals);
	MEMFREE(uv[0]);
	MEMFREE(uv[1]);
	MEMFREE(remap);
	vtx_capacity = 0;
}

void MBuf::reserve_indices(size_t num, bool shrink)
{
	assert(num > 0);

	if ((num <= idx_capacity) && (!shrink)) {
		return;
	}

	// printf("Reserving %zu indices\n", num);

	REALLOC_NUM(indices, num);
	idx_capacity = num;
}

void MBuf::reserve_vertices(size_t num, bool shrink)
{
	assert(num > 0);

	if ((num <= vtx_capacity) && (!shrink)) {
		return;
	}

	// printf("Reserving %zu vertices\n", num);

	if (true) {
		REALLOC_NUM(positions, num);
	}

	if (vtx_attr & VtxAttr::NML) {
		REALLOC_NUM(normals, num);
	}

	if (vtx_attr & VtxAttr::UV0) {
		REALLOC_NUM(uv[0], num);
	}

	if (vtx_attr & VtxAttr::UV1) {
		REALLOC_NUM(uv[1], num);
	}

	if (vtx_attr & VtxAttr::MAP) {
		REALLOC_NUM(remap, num);
	}

	vtx_capacity = num;
}
