#ifndef GL_GLEXT_PROTOTYPES
	#define GL_GLEXT_PROTOTYPES 1
#endif
#include "myosotis.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "math_utils.h"
#include "ndc.h"
#include "viewer.h"

static void resize_window_callback(GLFWwindow *window, int width, int height);

static void mouse_button_callback(GLFWwindow *window, int button, int action,
				  int mods);

static void cursor_position_callback(GLFWwindow *window, double xpos,
				     double ypos);

static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
			 int mods);

static void GL_debug_callback(GLenum source, GLenum type, GLuint id,
			      GLenum severity, GLsizei length,
			      const GLchar *message, const void *user_param)
{
	(void)source;
	(void)length;
	(void)user_param;
	(void)id;
	if (type == GL_DEBUG_TYPE_ERROR) {
		printf("GL CALLBACK: %s type = 0x%x, severity = 0x%x,\
			message = %s\n",
		       type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "",
		       type, severity, message);
	}
}

bool Myosotis::init(int width, int height)
{
	/* Set-up GLFW */
	if (!glfwInit()) {
		return (false);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	glfwWindowHint(GLFW_DEPTH_BITS, 32);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(width, height, "Myosotis", NULL, NULL);
	if (!window) {
		return (false);
	}

	glfwSetWindowUserPointer(window, this);
	glfwMakeContextCurrent(window);
	glDebugMessageCallback(GL_debug_callback, nullptr);

	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, resize_window_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSwapInterval(1);

	/* Set-up OpenGL for our choice of NDC */
	set_up_opengl_for_ndc();

	/* Set-up ImGUI */
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	io = &ImGui::GetIO();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(cfg.glsl_version);

	/* Set-up Viewer3D */
	viewer.init(width, height);

	return (true);
}

bool Myosotis::should_close() { return glfwWindowShouldClose(window); }

bool Myosotis::new_frame()
{
	glfwPollEvents();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::Begin("Controls");

	ImGui::Checkbox("Adaptative LOD", &cfg.adaptative_lod);

	ImGui::Checkbox("Continuous LOD", &cfg.continuous_lod);

	ImGui::Checkbox("Colorize LOD", &cfg.colorize_lod);

	ImGui::Checkbox("Colorize Cells", &cfg.colorize_cells);

	ImGui::Checkbox("Smooth shading", &cfg.smooth_shading);

	ImGui::Checkbox("Frustum cull", &cfg.frustum_cull);

	ImGui::Checkbox("Wireframe mode", &cfg.wireframe_mode);

	ImGui::Checkbox("Freeze drawn cells", &cfg.freeze_vp);

	if (ImGui::Checkbox("Use Vsync", &cfg.vsync)) {
		glfwSwapInterval(cfg.vsync);
	}

	if (ImGui::DragFloat("FOV", &cfg.camera_fov, 1, 5, 120, "%.0f")) {
		viewer.camera.set_fov(cfg.camera_fov);
	}

	ImGui::DragFloat("Pixel Error", &cfg.pix_error, 0.1, 0.5, 5, "%.1f");

	int &e = cfg.level;
	ImGui::RadioButton("Level 0", &e, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Level 1", &e, 1);
	ImGui::SameLine();
	ImGui::RadioButton("Level 2", &e, 2);
	ImGui::RadioButton("Level 3", &e, 3);
	ImGui::SameLine();
	ImGui::RadioButton("Level 4", &e, 4);
	ImGui::SameLine();
	ImGui::RadioButton("Level 5", &e, 5);
	ImGui::RadioButton("Level 6", &e, 6);
	ImGui::SameLine();
	ImGui::RadioButton("Level 7", &e, 7);
	ImGui::SameLine();
	ImGui::RadioButton("Level 8", &e, 8);

	ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate,
		    io->Framerate);

	ImGui::Text("Number of triangles : %d", stat.drawn_tris);

	ImGui::Text("Number of cells : %d", stat.drawn_cells);

	ImGui::End();
	ImGui::Render();

	return (true);
}

bool Myosotis::clean()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();

	return (true);
}

static void resize_window_callback(GLFWwindow *window, int width, int height)
{
	Myosotis *app = (Myosotis *)glfwGetWindowUserPointer(window);

	app->viewer.width = width;
	app->viewer.height = height;
	app->viewer.camera.set_aspect((float)width / height);

	glViewport(0, 0, width, height);
}

static void mouse_button_callback(GLFWwindow *window, int button, int action,
				  int mods)
{
	Myosotis *app = (Myosotis *)glfwGetWindowUserPointer(window);

	if (app->io->WantCaptureMouse) {
		return;
	}

	if (action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		app->viewer.mouse_pressed(xpos, ypos, button, mods);
	} else if (action == GLFW_RELEASE) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		app->viewer.mouse_released(button, mods);
	}
}

static void cursor_position_callback(GLFWwindow *window, double xpos,
				     double ypos)
{
	Myosotis *app = (Myosotis *)glfwGetWindowUserPointer(window);

	if (app->io->WantCaptureMouse) {
		return;
	}

	app->viewer.mouse_move(xpos, ypos);
}

static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	Myosotis *app = (Myosotis *)glfwGetWindowUserPointer(window);

	if (app->io->WantCaptureMouse) {
		return;
	}

	app->viewer.mouse_scroll(xoffset, yoffset);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
			 int mods)
{
	(void)scancode;
	(void)mods;
	Myosotis *app = (Myosotis *)glfwGetWindowUserPointer(window);

	if (app->io->WantCaptureKeyboard) {
		return;
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(app->window, 1);
		return;
	}

	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		app->cfg.smooth_shading ^= true;
		return;
	}

	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
		app->viewer.nav_mode = NavMode::Orbit;
		return;
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		app->viewer.nav_mode = NavMode::Free;
		return;
	}

	app->viewer.key_pressed(key, action);
}

float set_kappa(float screen_width, float mean_relative_error,
		float pixel_error, float fov)
{
	float tmp = screen_width * mean_relative_error /
		    (pixel_error * tan(fov * PI / 360));
	return tmp > 4 ? tmp : 4;
}
