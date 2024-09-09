#include <GLFW/glfw3.h>
#include <math.h>

#include "camera.h"
#include "trackball.h"
#include "transform.h"
#include "viewer.h"

#define DOUBLE_CLICK_TIME 0.5 /* in seconds           */
#define ZOOM_SENSITIVITY 0.3f /* for mouse wheel zoom */

bool Viewer3D::init(int width, int height)
{

	this->width = width;
	this->height = height;

	camera.set_aspect((float)width / height);
	camera.set_fov(45.f);

	return (true);
}

void Viewer3D::mouse_pressed(float px, float py, int button, int mods)
{
	static double last_pressed = -1.0;
	double now = glfwGetTime();
	bool double_click = (now - last_pressed) < DOUBLE_CLICK_TIME;
	last_pressed = now;

	is_mouse_pressed = true;
	this->button = button;
	this->mods = mods;
	last_click_x = px;
	last_click_y = py;
	last_camera_pos = camera.get_position();
	last_camera_rot = camera.get_rotation();
	last_trackball_v = screen_trackball(px, py, width, height);

	if (!double_click)
		return;

	float depth;
	float tx = px;
	float ty = height - py;
	glReadPixels(tx, ty, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	/* Don't track clicks outside of model */
	if (approx_equal(depth, 1.f)) {
		return;
	}

	target = camera.world_coord_at(px / width, py / height, depth);

	float dist = norm(target - camera.get_position());
	printf("Distance : %f\n", dist);
}

void Viewer3D::mouse_released(int button, int mods)
{
	(void)button;
	(void)mods;
	is_mouse_pressed = false;
}

void Viewer3D::mouse_move(float px, float py)
{
	if (!is_mouse_pressed)
		return;

	if (mods & GLFW_MOD_SHIFT) {
		/* Translation in x and y */
		float dist = norm(target - camera.get_position());
		float mult = dist / width;
		Vec3 trans;
		trans.x = (last_click_x - px) * mult;
		trans.y = (py - last_click_y) * mult;
		trans.z = 0.f;
		camera.set_position(last_camera_pos);
		camera.translate(trans, Camera::View);
	} else if (mods & GLFW_MOD_CONTROL) {
		/* Zoom (translation in target direction) */
		float mu = ZOOM_SENSITIVITY * (px - last_click_x) / 100;
		Vec3 new_pos = target + exp(mu) * (last_camera_pos - target);
		camera.set_position(new_pos);
	} else /* no modifier */ {
		Vec3 trackball_v = screen_trackball(px, py, width, height);
		Quat rot = great_circle_rotation(last_trackball_v, trackball_v);
		/* rot quat is in view frame, back to world frame */
		rot.xyz = rotate(rot.xyz, last_camera_rot);
		camera.set_position(last_camera_pos);
		camera.set_rotation(last_camera_rot);
		/* TODO adapt sensitivity to camera fov in free mode */
		rot = pow(rot, sensitivity);
		if (nav_mode == NavMode::Orbit) {
			camera.orbit(-rot, target);
		} else if (nav_mode == NavMode::Free) {
			camera.rotate(-rot);
		}
		/**
		 * Great_circle_rotation is singular when from and to are
		 * close to antipodal. To avoid that situation, we checkout
		 * mouse move when the from and to first belong to two opposite
		 * hemispheres.
		 */
		if (dot(trackball_v, last_trackball_v) < 0) {
			last_click_x = px;
			last_click_y = py;
			last_camera_pos = camera.get_position();
			last_camera_rot = camera.get_rotation();
			last_trackball_v = trackball_v;
		}
	}
}

void Viewer3D::mouse_scroll(float xoffset, float yoffset)
{
	(void)xoffset;
	Vec3 old_pos = camera.get_position();
	Vec3 new_pos =
	    target + exp(-ZOOM_SENSITIVITY * yoffset) * (old_pos - target);
	camera.set_position(new_pos);
}

void Viewer3D::key_pressed(int key, int action)
{
	(void)key;
	(void)action;
}
