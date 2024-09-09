include_directories(
	${Myosotis_SOURCE_DIR}/include/
	)

add_executable(myosotis
	main.cpp
	viewer.cpp
	camera.cpp
	frustum.cpp
	trackball.cpp
	mesh_io.cpp
	mesh_grid.cpp
	mesh_utils.cpp
	mesh_stats.cpp
	mesh_optimize.cpp
	vertex_remap.cpp
	mesh.cpp
	chrono.cpp
	shaders.cpp
	myosotis.cpp
	)

add_library(miniply
	../extern/miniply/miniply.cpp
	)

add_library(meshoptimizer
	../extern/meshoptimizer/src/allocator.cpp
	../extern/meshoptimizer/src/indexgenerator.cpp
	../extern/meshoptimizer/src/simplifier_mod.cpp
	../extern/meshoptimizer/src/overdrawanalyzer.cpp
	../extern/meshoptimizer/src/overdrawoptimizer.cpp
	../extern/meshoptimizer/src/vcacheanalyzer.cpp
	../extern/meshoptimizer/src/vcacheoptimizer.cpp
	../extern/meshoptimizer/src/vfetchanalyzer.cpp
	../extern/meshoptimizer/src/vfetchoptimizer.cpp
	)

add_library(imgui_glfw_opengl3
	../extern/imgui/imgui.cpp
	../extern/imgui/imgui_draw.cpp
	../extern/imgui/imgui_tables.cpp
	../extern/imgui/imgui_widgets.cpp
	../extern/imgui/imgui_impl_glfw.cpp
	../extern/imgui/imgui_impl_opengl3.cpp
	)


#add_library(meshoptimizer STATIC IMPORTED)
#set_target_properties(meshoptimizer PROPERTIES
#	IMPORTED_LOCATION
#	"${CMAKE_SOURCE_DIR}/extern/meshoptimizer/build/libmeshoptimizer.a"
#	)

target_compile_features(myosotis PRIVATE cxx_std_17)
target_compile_options(myosotis PRIVATE -Wall -Wextra)
target_link_libraries(myosotis miniply)
target_link_libraries(myosotis meshoptimizer)
target_link_libraries(myosotis imgui_glfw_opengl3)
target_link_libraries(myosotis OpenGL)
target_link_libraries(myosotis glfw)
target_link_libraries(myosotis pthread)
target_link_libraries(myosotis ${CMAKE_DL_LIBS})

