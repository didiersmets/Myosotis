#pragma once

#include <stdint.h>

#include "array.h"
#include "camera.h"
#include "hash.h"
#include "hash_table.h"
#include "mesh.h"
#include "vec3.h"

union alignas(8) CellCoord {
	struct {
		int16_t lod;
		int16_t x;
		int16_t y;
		int16_t z;
	};
	uint64_t key;
};

inline bool operator==(CellCoord c1, CellCoord c2)
{
	return (c1.key == c2.key);
}

struct CellCoordHasher {
	static constexpr CellCoord empty_key = {{-1, 0, 0, 0}};
	size_t hash(CellCoord coord) const;
	bool is_empty(CellCoord coord) const;
	bool is_equal(CellCoord c1, CellCoord c2) const;
};
typedef HashTable<CellCoord, uint32_t, CellCoordHasher> CellTable;

inline size_t CellCoordHasher::hash(CellCoord coord) const
{
	return murmur2_64(0, coord.key);
}

inline bool CellCoordHasher::is_empty(CellCoord coord) const
{
	return (coord == empty_key);
}

inline bool CellCoordHasher::is_equal(CellCoord c1, CellCoord c2) const
{
	return (c1 == c2);
}

CellCoord parent_coord(const CellCoord coord);

struct MeshGrid {
	/* Grid */
	Vec3 base;
	float step;
	/* Data holding meshlets */
	MBuf data;
	uint32_t next_index_offset = 0;
	uint32_t next_vertex_offset = 0;
	/* Arrays, one entry per cell */
	TArray<CellCoord> cell_coords;
	TArray<Mesh> cells;
	TArray<float> cell_errors;
	float mean_relative_error;
	/* Facilities to access or query meshlets */
	uint32_t levels;
	float err_tol;
	TArray<uint32_t> cell_offsets;
	TArray<uint32_t> cell_counts;
	CellTable cell_table;
	/* Methods */
	MeshGrid(Vec3 base, float step, uint32_t levels, float err_tol);
	Mesh *get_cell(CellCoord ccoord);
	unsigned get_children(CellCoord pcoord, Mesh *children[8]);
	void build_from_mesh(const MBuf &src, const Mesh &mesh,
			     int num_threads = 1);
	void init_from_mesh(const MBuf &src, const Mesh &mesh);
	void build_level(uint32_t level, uint8_t num_threads = 1);
	void build_parent_cell(CellCoord pcoord);
	void compute_mean_relative_error();
	enum Visibility get_visibility(const float *pvm, CellCoord coord);
	bool cell_is_acceptable(const Vec3 &vp, uint32_t idx,
				bool continuous_lod, float error_multiplier);
	void select_cells_from_view_point(const Vec3 &vp,
					  float error_multiplier,
					  bool continuous_lod,
					  bool frustum_cull, const float *pvm,
					  TArray<uint32_t> &to_draw,
					  TArray<uint32_t> &parents);
	float cell_view_ratio_dinf(const Vec3 vp, CellCoord coord);
	float cell_view_ratio_d2(const Vec3 vp, CellCoord coord);
	uint32_t get_triangle_count(uint32_t level);
	uint32_t get_vertex_count(uint32_t level);
};

