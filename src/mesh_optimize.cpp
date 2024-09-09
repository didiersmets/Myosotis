#include "meshoptimizer/src/meshoptimizer.h"

#include "mesh.h"
#include "array.h"

#include "mesh_optimize.h"

void meshopt_optimize(MBuf& data, const Mesh& mesh)
{
	uint32_t *idx = data.indices + mesh.index_offset;
	float *pos = (float*)(data.positions + mesh.vertex_offset);
	float *nml = (float*)(data.normals + mesh.vertex_offset);
	float *uv0 = (float*)(data.uv[0] + mesh.vertex_offset);
	uint32_t nidx = mesh.index_count;
	uint32_t nvtx = mesh.vertex_count;
		
	meshopt_optimizeVertexCache(idx, idx, nidx, nvtx);
		
	const float kThreshold = 1.01f;
	meshopt_optimizeOverdraw(idx, idx, nidx, pos, nvtx, sizeof(Vec3), 
				 kThreshold);

	TArray<unsigned int> remap(nvtx);
	meshopt_optimizeVertexFetchRemap(&remap[0], idx, nidx, nvtx);
	meshopt_remapIndexBuffer(idx, idx, nidx, &remap[0]);
	meshopt_remapVertexBuffer(pos, pos, nvtx, sizeof(Vec3), &remap[0]);
	if (data.vtx_attr & VtxAttr::NML)
	{
		meshopt_remapVertexBuffer(nml, nml, nvtx, sizeof(Vec3), 
					  &remap[0]);
	}
	if (data.vtx_attr & VtxAttr::UV0)
	{
		meshopt_remapVertexBuffer(uv0, uv0, nvtx, sizeof(Vec2),
					  &remap[0]);
	}
}

