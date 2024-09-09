#ifdef DEBUG
	#include <stdio.h>
#endif
#include <string.h>

#define FAST_OBJ_IMPLEMENTATION 1
#include "fast_obj/fast_obj.h"
#undef FAST_OBJ_IMPLEMENTATION

#include "miniply/miniply.h"

#include "array.h"
#include "hash.h"
#include "hash_table.h"
#include "mesh.h"
#include "mesh_io.h"
#include "vec2.h"
#include "vec3.h"

struct ObjVertexHasher {
	bool has_normals;
	bool has_uv;
	const Vec3 *pos;
	const Vec3 *nml;
	const Vec2 *uv;
	static constexpr fastObjIndex empty_key = {0, 0, 0};
	size_t hash(fastObjIndex key) const;
	bool is_empty(fastObjIndex key) const;
	bool is_equal(fastObjIndex key1, fastObjIndex key2) const;
};
typedef HashTable<fastObjIndex, uint32_t, ObjVertexHasher> ObjVertexTable;

inline size_t ObjVertexHasher::hash(fastObjIndex key) const
{
	return position_hash((const float *)(pos + key.p));
}

bool ObjVertexHasher::is_empty(fastObjIndex key) const { return (key.p == 0); }

bool ObjVertexHasher::is_equal(fastObjIndex key1, fastObjIndex key2) const
{
	bool res = (pos[key1.p] == pos[key2.p]);
	if (res && has_normals)
		res &= (nml[key1.n] == nml[key2.n]);
	if (res && has_uv)
		res &= (uv[key1.t] == uv[key2.t]);

	return (res);
}

static int load_obj(const fastObjMesh &obj, MBuf &data, Mesh &mesh)
{
	data.clear();
	data.vtx_attr = VtxAttr::P;

	size_t obj_vertex_count = 0;
	size_t index_count = 0;

	for (unsigned int i = 0; i < obj.face_count; ++i) {
		obj_vertex_count += obj.face_vertices[i];
		index_count += 3 * (obj.face_vertices[i] - 2);
	}

	data.reserve_indices(index_count);

	bool has_normals = false;
	bool has_uv = false;
	for (size_t i = 0; i < obj_vertex_count; ++i) {
		has_normals |= (obj.indices[i].n != 0);
		has_uv |= (obj.indices[i].t != 0);
	}
	data.vtx_attr |= has_normals ? VtxAttr::NML : 0;
	data.vtx_attr |= has_uv ? VtxAttr::UV0 : 0;

	size_t vertex_count_guess = index_count / 6;
	vertex_count_guess += vertex_count_guess / 2;
	data.reserve_vertices(vertex_count_guess);

	/* Discover vertices and encode indices */

	size_t idx = 0;
	size_t idx_offset = 0;

	size_t vertex_count = 0;
	ObjVertexHasher hasher{has_normals, has_uv, (const Vec3 *)obj.positions,
			       (const Vec3 *)obj.normals,
			       (const Vec2 *)obj.texcoords};
	ObjVertexTable vertices(vertex_count_guess, hasher);

	for (size_t i = 0; i < obj.face_count; ++i) {
		size_t idx_start = idx;
		for (size_t j = 0; j < obj.face_vertices[i]; ++j) {
			if (j >= 3) /* Triangulate */
			{
				data.indices[idx + 0] = data.indices[idx_start];
				data.indices[idx + 1] = data.indices[idx - 1];
				idx += 2;
			}

			fastObjIndex pnt = obj.indices[idx_offset + j];

			uint32_t *p = vertices.get_or_set(pnt, vertex_count);
			if (!p) {
				data.indices[idx] = vertex_count;

				/* Copy vertex */
				if (vertex_count >= data.vtx_capacity) {
					size_t new_cap = data.vtx_capacity +
							 data.vtx_capacity / 2;
#ifdef DEBUG
					printf("Reserve %zu vertices\n",
					       new_cap);
#endif
					data.reserve_vertices(new_cap);
				}
				void *dst;
				void *src;
				dst = data.positions + vertex_count;
				src = obj.positions + 3 * pnt.p;
				memcpy(dst, src, 3 * sizeof(float));
				if (has_normals) {
					dst = data.normals + vertex_count;
					src = obj.normals + 3 * pnt.n;
					memcpy(dst, src, 3 * sizeof(float));
				}
				if (has_uv) {
					dst = data.uv[0] + vertex_count;
					src = obj.texcoords + 2 * pnt.t;
					memcpy(dst, src, 2 * sizeof(float));
				}
				vertex_count++;
			} else {
				data.indices[idx] = *p;
			}
			idx++;
		}
		idx_offset += obj.face_vertices[i];
	}

	mesh.index_offset = 0;
	mesh.vertex_offset = 0;
	mesh.index_count = index_count;
	mesh.vertex_count = vertex_count;

	bool shrink = true;
	data.reserve_vertices(vertex_count, shrink);

	return (EXIT_SUCCESS);
}

int load_obj(const char *filename, MBuf &data, Mesh &mesh)
{
	fastObjMesh *obj = fast_obj_read(filename);
	if (obj == nullptr) {
		return (EXIT_FAILURE);
	}
	int res = load_obj(*obj, data, mesh);
	fast_obj_destroy(obj);

	return (res);
}

int load_ply(const char *filename, MBuf &data, Mesh &mesh)
{
	using namespace miniply;
	PLYReader reader(filename);
	if (!reader.valid()) {
		return (EXIT_FAILURE);
	}

	bool got_verts = false;
	bool got_faces = false;

	while (reader.has_element() && (!got_verts || !got_faces)) {
		if (reader.element_is(kPLYVertexElement)) {
			uint32_t pos_idx[3];
			reader.load_element();
			reader.find_pos(pos_idx);
			uint32_t vertex_count = reader.num_rows();
			mesh.vertex_offset = 0;
			mesh.vertex_count = vertex_count;

			uint32_t nml_idx[3];
			if (reader.find_normal(nml_idx)) {
				data.vtx_attr |= VtxAttr::NML;
			}

			uint32_t uv_idx[2];
			if (reader.find_texcoord(uv_idx)) {
				data.vtx_attr |= VtxAttr::UV0;
			}

			data.reserve_vertices(vertex_count);

			reader.extract_properties(
			    pos_idx, 3, PLYPropertyType::Float, data.positions);
			if (data.vtx_attr & VtxAttr::NML) {
				reader.extract_properties(
				    nml_idx, 3, PLYPropertyType::Float,
				    data.normals);
			}
			if (data.vtx_attr & VtxAttr::UV0) {
				reader.extract_properties(
				    uv_idx, 2, PLYPropertyType::Float,
				    data.uv[0]);
			}
			got_verts = true;
		} else if (reader.element_is(kPLYFaceElement)) {
			uint32_t idx[1];
			reader.load_element();
			reader.find_indices(idx);
			bool polys = reader.requires_triangulation(idx[0]);
			if (polys && !got_verts) {
				fprintf(stderr, "Error: need vertex \
					positions to triangulate faces.\n");
				break;
			}
			uint32_t index_count;
			if (polys) {
				index_count = reader.num_triangles(idx[0]) * 3;
				data.reserve_indices(index_count);
				reader.extract_triangles(
				    idx[0], (const float *)data.positions,
				    mesh.vertex_count, PLYPropertyType::Int,
				    data.indices);
			} else {
				index_count = reader.num_rows() * 3;
				data.reserve_indices(index_count);
				reader.extract_list_property(
				    idx[0], PLYPropertyType::Int, data.indices);
			}
			mesh.index_offset = 0;
			mesh.index_count = index_count;
			got_faces = true;
		}
		if (got_verts && got_faces) {
			break;
		}
		reader.next_element();
	}

	if (!got_verts || !got_faces) {
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}
