#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#ifndef GL_GLEXT_PROTOTYPES
	#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>

#include "aabb.h"
#include "chrono.h"
#include "mesh_grid.h"
#include "mesh_io.h"
#include "mesh_optimize.h"
#include "mesh_stats.h"
#include "mesh_utils.h"
#include "myosotis.h"
#include "shaders.h"
#include "transform.h"
#include "version.h"
#include "viewer.h"

#define TARGET_CELL_IDX_COUNT (1 << 16)
#define ERR_TOL 0.01

void syntax(char *argv[])
{
	printf("Syntax : %s mesh_file_name [max_level]\n", argv[0]);
}

int main(int argc, char **argv)
{
	if (argc <= 1) {
		syntax(argv);
		return (EXIT_FAILURE);
	}

	/* Load and process mesh to build mesh_grid */
	timer_start();

	MBuf data;
	Mesh mesh;
	{
		size_t len = strlen(argv[1]);
		const char *ext = argv[1] + (len - 3);
		if (strncmp(ext, "obj", 3) == 0) {
			if (load_obj(argv[1], data, mesh)) {
				printf("Error reading Wavefront file.\n");
				return (EXIT_FAILURE);
			}
		} else if (strncmp(ext, "ply", 3) == 0) {
			if (load_ply(argv[1], data, mesh)) {
				printf("Error reading PLY file.\n");
				return (EXIT_FAILURE);
			}
		} else {
			printf("Unsupported (yet) file type extension: %s\n",
			       ext);
			return (EXIT_FAILURE);
		}
	}

	printf("Triangles : %d Vertices : %d\n", mesh.index_count / 3,
	       mesh.vertex_count);
	timer_stop("loading mesh");

	/* Input mesh stat and optimization */
	if (argc > 4 && *argv[4] == '1') {
		timer_start();
		meshopt_statistics("Raw", data, mesh);
		timer_start();
		meshopt_optimize(data, mesh);
		timer_stop("optimize mesh");
		meshopt_statistics("Optimized", data, mesh);
	}

	/* Computing mesh normals */
	if (!(data.vtx_attr & VtxAttr::NML)) {
		timer_start();
		printf("Computing normals.\n");
		compute_mesh_normals(mesh, data);
		timer_stop("compute_mesh_normals");
	}

	/* Computing mesh bounds */
	timer_start();
	Aabb bbox = compute_mesh_bounds(mesh, data);
	Vec3 model_center = (bbox.min + bbox.max) * 0.5f;
	Vec3 model_extent = (bbox.max - bbox.min);
	float model_size = max(model_extent);
	printf("Model size : %f\n", model_size);
	timer_stop("compute_mesh_bounds");

	/* Building mesh_grid */
	timer_start();
	int max_level;
	if (argc > 2) {
		max_level = atoi(argv[2]);
	} else {
		max_level = 0;
		while ((1ul << (2 * max_level + 2)) * TARGET_CELL_IDX_COUNT <
		       mesh.index_count) {
			max_level += 1;
			if (max_level == 15)
				break;
		}
		printf(
		    "Maximum octree level unspecified. Using %d based on mesh "
		    "index count.\n",
		    max_level);
	}
	float err_tol;
	if (argc > 3) {
		err_tol = atof(argv[3]);
	} else {
		err_tol = ERR_TOL;
	}
	float step = model_size / (1 << max_level);
	Vec3 base = bbox.min;
	MeshGrid mg(base, step, max_level, err_tol);
	mg.build_from_mesh(data, mesh, 8);
	timer_stop("split_mesh_with_grid");

	/* Dispose original mesh */
	data.clear();

	/* Main window and context */
	Myosotis app;

	if (!app.init(1920, 1080)) {
		return (EXIT_FAILURE);
	}

	/* Init camera position */
	app.viewer.target = model_center;
	Vec3 start_pos = (model_center + 2.f * Vec3(0, 0, model_size));
	app.viewer.camera.set_position(start_pos);
	app.viewer.camera.set_near(0.0001 * model_size);
	app.viewer.camera.set_far(1000 * model_size);

	glEnable(GL_DEBUG_OUTPUT);

	/* Upload mesh grid */

	/* Mesh grid Index buffer */
	GLuint mg_idx;
	glGenBuffers(1, &mg_idx);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mg_idx);
	printf("Allocating %zuMb for indices\n",
	       mg.data.idx_capacity * sizeof(uint32_t) / (1 << 20));
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		     mg.data.idx_capacity * sizeof(uint32_t), mg.data.indices,
		     GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/* Mesh grid Position buffer */
	GLuint mg_pos;
	glGenBuffers(1, &mg_pos);
	glBindBuffer(GL_ARRAY_BUFFER, mg_pos);
	printf("Allocating %zuMb for positions\n",
	       mg.data.vtx_capacity * sizeof(Vec3) / (1 << 20));
	glBufferData(GL_ARRAY_BUFFER, mg.data.vtx_capacity * sizeof(Vec3),
		     mg.data.positions, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* Mesh grid Normal buffer */
	GLuint mg_nml;
	glGenBuffers(1, &mg_nml);
	glBindBuffer(GL_ARRAY_BUFFER, mg_nml);
	printf("Allocating %zuMb for normals\n",
	       mg.data.vtx_capacity * sizeof(Vec3) / (1 << 20));
	glBufferData(GL_ARRAY_BUFFER, mg.data.vtx_capacity * sizeof(Vec3),
		     mg.data.normals, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* Mesh grid Vertex Parent Idx buffer */
	GLuint mg_par;
	glGenBuffers(1, &mg_par);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mg_par);
	printf("Allocating %zuMb for parent index\n",
	       mg.data.vtx_capacity * sizeof(uint32_t) / (1 << 20));
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		     mg.data.vtx_capacity * sizeof(uint32_t), mg.data.remap,
		     GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/* Setup VAOs */

	/* Mesh grid Default VAO */
	GLuint mg_default_vao;
	glGenVertexArrays(1, &mg_default_vao);
	glBindVertexArray(mg_default_vao);
	glBindBuffer(GL_ARRAY_BUFFER, mg_pos);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT),
			      (void *)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mg_nml);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT),
			      (void *)0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, mg_par);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT,
			       1 * sizeof(GL_UNSIGNED_INT), (void *)0);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mg_idx);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* Vertex fetch VAO */
	GLuint fetch_vao;
	glGenVertexArrays(1, &fetch_vao);
	glBindVertexArray(fetch_vao);
	glBindBuffer(GL_ARRAY_BUFFER, mg_par);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT,
			       1 * sizeof(GL_UNSIGNED_INT), (void *)0);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mg_idx);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/* Setup programs */

	GLint mesh_prg =
	    create_shader("./shaders/default.vert", "./shaders/default.frag");
	if (mesh_prg < 0) {
		return EXIT_FAILURE;
	}

	// GLint fetch_mesh_prg = create_shader("./shaders/fetch_mesh.vert",
	//				  "./shaders/default.frag");
	// if (fetch_mesh_prg < 0)
	//{
	//	return EXIT_FAILURE;
	// }

	// GLint nml_prg = create_shader("./shaders/face_normals.vert",
	//				  "./shaders/face_normals.frag");
	// if (nml_prg < 0)
	//{
	//	return EXIT_FAILURE;
	// }

	/* Setup some rendering options */

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Rendering loop */
	printf("Starting rendering loop\n");
	TArray<uint32_t> to_draw;
	TArray<uint32_t> parents;
	while (!app.should_close()) {
		app.new_frame();

		glClearColor(app.cfg.clear_color.x, app.cfg.clear_color.y,
			     app.cfg.clear_color.z, app.cfg.clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (app.cfg.wireframe_mode) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		} else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		/* Update uniform data */
		Mat4 proj = app.viewer.camera.view_to_clip();
		Mat4 vm = app.viewer.camera.world_to_view();
		Vec3 camera_pos = app.viewer.camera.get_position();
		if (app.cfg.level > max_level)
			app.cfg.level = max_level;

		/* Draw mesh */
		if (app.cfg.adaptative_lod) {
			/* Set kappa */

			float error_multiplier =
			    4 * app.viewer.width /
			    (app.cfg.pix_error *
			     tan(app.cfg.camera_fov * PI / 360));

			float kappa = error_multiplier * mg.mean_relative_error;
			printf("Kappa : %f\r", kappa);
			fflush(stdout);

			if (!app.cfg.freeze_vp) {
				Vec3 vp = app.viewer.camera.get_position();
				Mat4 proj_vm =
				    app.viewer.camera.world_to_clip();
				float *pvm = &proj_vm(0, 0);
				to_draw.clear();
				parents.clear();

				// timer_start();
				mg.select_cells_from_view_point(
				    vp, error_multiplier,
				    app.cfg.continuous_lod,
				    app.cfg.frustum_cull, pvm, to_draw,
				    parents);
				// timer_stop("Selection");
				app.stat.drawn_cells = to_draw.size;
			}

			glUseProgram(mesh_prg);
			glBindVertexArray(mg_default_vao);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mg_pos);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mg_nml);
			glUniformMatrix4fv(0, 1, 0, &(vm.cols[0][0]));
			glUniformMatrix4fv(1, 1, 0, &(proj.cols[0][0]));
			glUniform3fv(2, 1, &camera_pos[0]);
			glUniform1i(3, app.cfg.continuous_lod);
			glUniform1i(4, app.cfg.wireframe_mode ||
					   app.cfg.smooth_shading);
			glUniform1i(5, app.cfg.colorize_lod);
			glUniform1i(6, app.cfg.colorize_cells);
			glUniform1f(7, kappa);
			glUniform1f(8, mg.step);

			app.stat.drawn_tris = 0;

			for (int i = to_draw.size - 1; i >= 0; --i) {
				Mesh &mesh = mg.cells[to_draw[i]];
				Mesh &pmesh = mg.cells[parents[i]];
				CellCoord coord = mg.cell_coords[to_draw[i]];
				glUniform1i(9, coord.lod);
				glUniform1i(10, coord.x);
				glUniform1i(11, coord.y);
				glUniform1i(12, coord.z);
				glUniform1i(13, mesh.vertex_offset);
				glUniform1i(14, pmesh.vertex_offset);
				glDrawElementsBaseVertex(
				    GL_TRIANGLES, mesh.index_count,
				    GL_UNSIGNED_INT,
				    (void *)(mesh.index_offset *
					     sizeof(uint32_t)),
				    mesh.vertex_offset);
				app.stat.drawn_tris += mesh.index_count / 3;
			}
			glBindVertexArray(0);
		} else {
			glUseProgram(mesh_prg);
			glBindVertexArray(mg_default_vao);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mg_pos);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mg_nml);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mg_par);
			glUniformMatrix4fv(0, 1, 0, &(vm.cols[0][0]));
			glUniformMatrix4fv(1, 1, 0, &(proj.cols[0][0]));
			glUniform3fv(2, 1, &camera_pos[0]);
			glUniform1i(
			    3, 0); /* No continuous LOD is fixed LOD mode */
			glUniform1i(4, app.cfg.wireframe_mode ||
					   app.cfg.smooth_shading);
			glUniform1i(5, app.cfg.colorize_lod);
			int cell_counts = mg.cell_counts[app.cfg.level];
			int cell_offset = mg.cell_offsets[app.cfg.level];

			app.stat.drawn_tris = 0;

			for (int i = 0; i < cell_counts; ++i) {
				Mesh &mesh = mg.cells[cell_offset + i];
				glDrawElementsBaseVertex(
				    GL_TRIANGLES, mesh.index_count,
				    GL_UNSIGNED_INT,
				    (void *)(mesh.index_offset *
					     sizeof(uint32_t)),
				    mesh.vertex_offset);
				app.stat.drawn_tris += mesh.index_count / 3;
			}
			glBindVertexArray(0);
		}

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(app.window);
	}

	/* Cleaning */
	app.clean();

	return (EXIT_SUCCESS);
}
