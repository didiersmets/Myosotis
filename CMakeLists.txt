cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(Myosotis VERSION 0.1)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
set( CMAKE_VERBOSE_MAKEFILE OFF )
set( CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE )

configure_file(
	${PROJECT_SOURCE_DIR}/config/version.h.in
	${PROJECT_BINARY_DIR}/version.h
	)

include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/extern/
	)

find_package(OpenGL REQUIRED)
add_compile_definitions(DEBUG)
 
find_package(glfw3 REQUIRED)

add_subdirectory(
	src
	)



