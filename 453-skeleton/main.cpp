//Course: CPSC 453
//Assignment: 4
//Author: Cole Pawliw
//Date: December 4, 2022

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#define GLM_SWIZZLE
#include <math.h>
#include <cmath>

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <limits>
#include <functional>
#include <ctime>

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Texture.h"
#include "Window.h"
#include "Camera.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

bool play = true, reset = false;
float speed = 1.0f;

struct Planet;
void sphere(std::vector<glm::vec3>& points, std::vector<glm::vec2>& tex);
void assign_bodies(std::vector<Planet*>& bodies, glm::vec4& origin);
void restart(std::vector<Planet*>& bodies);

// EXAMPLE CALLBACKS
class Assignment4 : public CallbackInterface {

public:
	Assignment4()
		: camera(glm::radians(45.f), glm::radians(45.f), 3.0)
		, aspect(1.0f)
		, rightMouseDown(false)
		, mouseOldX(0.0)
		, mouseOldY(0.0)
	{}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_R && action == GLFW_PRESS) {
			reset = true;
		}
		else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
			play = !play;
		}
		else if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			speed -= 0.1;
			if (speed < 0.0f) {
				speed = 0.0f;
			}
		}
		else if (key == GLFW_KEY_D && action == GLFW_PRESS) {
			speed += 0.1;
			if (speed > 1.0f) {
				speed = 1.0f;
			}
		}
	}
	virtual void mouseButtonCallback(int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (action == GLFW_PRESS)			rightMouseDown = true;
			else if (action == GLFW_RELEASE)	rightMouseDown = false;
		}
	}
	virtual void cursorPosCallback(double xpos, double ypos) {
		if (rightMouseDown) {
			camera.incrementTheta(ypos - mouseOldY);
			camera.incrementPhi(xpos - mouseOldX);
		}
		mouseOldX = xpos;
		mouseOldY = ypos;
	}
	virtual void scrollCallback(double xoffset, double yoffset) {
		camera.incrementR(yoffset / 5);
	}
	virtual void windowSizeCallback(int width, int height) {
		// The CallbackInterface::windowSizeCallback will call glViewport for us
		CallbackInterface::windowSizeCallback(width,  height);
		aspect = float(width)/float(height);
	}

	void viewPipeline(ShaderProgram &sp, glm::mat4 M, glm::mat3 R, int light) {
		glm::mat4 V = camera.getView();
		glm::mat4 P = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.f);
		glm::vec3 view = camera.getPos();
		glm::vec3 origin(0.f, 0.f, 0.f);
		glm::vec3 white(1.f, 1.f, 1.f);

		GLint location = glGetUniformLocation(sp, "view_pos");
		glUniform3fv(location, 1, glm::value_ptr(view));
		location = glGetUniformLocation(sp, "light_pos");
		glUniform3fv(location, 1, glm::value_ptr(origin));
		location = glGetUniformLocation(sp, "light_col");
		glUniform3fv(location, 1, glm::value_ptr(white));

		GLint uniMat = glGetUniformLocation(sp, "M");
		glUniformMatrix4fv(uniMat, 1, GL_FALSE, glm::value_ptr(M));
		uniMat = glGetUniformLocation(sp, "V");
		glUniformMatrix4fv(uniMat, 1, GL_FALSE, glm::value_ptr(V));
		uniMat = glGetUniformLocation(sp, "P");
		glUniformMatrix4fv(uniMat, 1, GL_FALSE, glm::value_ptr(P));
		uniMat = glGetUniformLocation(sp, "norm_R");
		glUniformMatrix3fv(uniMat, 1, GL_FALSE, glm::value_ptr(R));

		GLint lighting_location = glGetUniformLocation(sp, "light");
		glUniform1i(lighting_location, light);
	}
	Camera camera;
private:
	bool rightMouseDown;
	float aspect;
	double mouseOldX;
	double mouseOldY;
};

struct Planet {
	// Struct's constructor deals with the texture.
	// Also sets default position, theta, scale, and transformationMatrix
	Planet(std::string texturePath, GLenum textureInterpolation, glm::vec4 initPos, float tilt, glm::vec4* parent, float scale,
		float theta, float phi) :
		texture(texturePath, textureInterpolation),
		init_position(initPos),
		curr_position(initPos),
		orbital_angle(0),
		axial_tilt(tilt),
		axial_rotation(0),
		scale(scale),
		transformationMatrix(1.0f), // This constructor sets it as the identity matrix
		normMatrix(1.0f),
		parentPos(*parent),
		orbit_rate(theta),
		rotate_rate(phi)
	{}

	void setTransformation() {
		glm::mat4 scaling, rotation, translation;
		update_pos();
		scaling = glm::mat4(scale, 0.f, 0.f, 0.f, //Scale the sphere
							0.f, scale, 0.f, 0.f,
							0.f, 0.f, scale, 0.f,
							0.f, 0.f, 0.f, 1.f);
		rotation = glm::mat4(sin(axial_rotation), 0.f, cos(axial_rotation), 0.f, //Rotate around the axis
										0.f, 1.f, 0.f, 0.f,
										cos(axial_rotation), 0.f, -sin(axial_rotation), 0.f,
										0.f, 0.f, 0.f, 1.f);
		rotation = glm::mat4(cos(axial_tilt), sin(axial_tilt), 0.f, 0.f, //Tilt the axis
							-sin(axial_tilt), cos(axial_tilt), 0.f, 0.f,
							0.f, 0.f, 1.f, 0.f,
							0.f, 0.f, 0.f, 1.f) * rotation;
		translation = glm::mat4(1.f, 0.f, 0.f, 0.f, //Move to position
								0.f, 1.f, 0.f, 0.f,
								0.f, 0.f, 1.f, 0.f,
								curr_position.x, curr_position.y, curr_position.z, 1.f);

		normMatrix = glm::mat3(rotation);
		transformationMatrix = translation * rotation * scaling;
	}

	void update_pos() {
		curr_position = glm::mat4(sin(orbital_angle), 0.f, cos(orbital_angle), 0.f,
			0.f, 1.f, 0.f, 0.f,
			cos(orbital_angle), 0.f, -sin(orbital_angle), 0.f,
			0.f, 0.f, 0.f, 1.f) * init_position;
		curr_position += parentPos;
	}

	void change_ang(double time) {
		orbital_angle += orbit_rate * time;
		axial_rotation += rotate_rate * time;

		if (orbital_angle > 2 * M_PI) {
			orbital_angle -= 2 * M_PI;
		}
		if (axial_rotation > 2 * M_PI) {
			axial_rotation -= 2 * M_PI;
		}

		setTransformation();
	}

	CPU_Geometry cgeom;
	GPU_Geometry ggeom;
	Texture texture;

	const glm::vec4 &parentPos;

	glm::vec4 init_position, curr_position; // Object's initial position, relative to it's parents current position
	float orbital_angle; // Angle the object has moved around it's orbit
	float axial_tilt; // Angle the object is tilted on it's axis.  DOES NOT CHANGE
	float axial_rotation; // Angle the object has rotated about it's axis
	float scale; // Object's scale
	float orbit_rate, rotate_rate;
	glm::mat4 transformationMatrix;
	glm::mat3 normMatrix;
};

int main() {
	Log::debug("Starting main");

	glfwSetTime(0.0);
	double last_time = 0.0, curr_time, diff_time;

	// WINDOW
	glfwInit();
	Window window(800, 800, "CPSC 453 - Assignment 3");

	GLDebug::enable();

	// CALLBACKS
	auto a4 = std::make_shared<Assignment4>();
	window.setCallbacks(a4);

	ShaderProgram shader("shaders/test.vert", "shaders/test.frag");

	glm::vec4 fake_parent(0.f, 0.f, 0.f, 1.f);

	std::vector<Planet*> bodies;
	assign_bodies(bodies, fake_parent);

	// RENDER LOOP
	while (!window.shouldClose()) {
		glfwPollEvents();

		if (reset) {
			restart(bodies);
			reset = false;
		}


		curr_time = glfwGetTime();
		diff_time = curr_time - last_time;
		last_time = curr_time;
		if (play) {
			for (int i = 0; i < 4; i++) {
				bodies[i]->change_ang(diff_time * speed);
			}
		}

		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL /*GL_LINE*/);

		shader.use();

		int lighting_mode;
		for (int i = 0; i < 4; i++) {
			lighting_mode = i % 3;
			a4->viewPipeline(shader, bodies[i]->transformationMatrix, bodies[i]->normMatrix, lighting_mode);
			bodies[i]->ggeom.bind();
			bodies[i]->texture.bind();
			glDrawArrays(GL_TRIANGLES, 0, GLsizei(bodies[i]->cgeom.verts.size()));
			bodies[i]->texture.unbind();
		}

		glDisable(GL_FRAMEBUFFER_SRGB); // disable sRGB for things like imgui
		window.swapBuffers();
	}
	glfwTerminate();
	return 0;
}

void sphere(std::vector<glm::vec3>& points, std::vector<glm::vec2>& tex) {
	//Used to generate the curve once to use all the points
	std::vector<glm::vec3> last_points, current_points, curve;
	float delta = 5 * (float) M_PI / 180.0f;
	float curr_u = 0.5f, last_u, curr_v, last_v;


	points.clear();
	tex.clear();

	//Make a half circle of points
	for (float y = -1.f; y <= 1.f; y += 0.02f) {
		float x = sqrt(1.f - y * y);
		curve.push_back(glm::vec3(x, y, x));
	}

	current_points = curve;
	for (int i = 0; i < current_points.size(); i++) {
		current_points[i].z = 0.f;
	}

	//Rotate the half circle about the y-axis, storing triangular points
	for (float v = 0; v <= 2 * (float) M_PI; v += delta) {
		last_points = current_points;
		curr_v = 0.f;
		for (int i = 0; i < curve.size(); i++) {
			current_points[i] = curve[i] * glm::vec3(cos(v), 1.f, sin(v));
		}

		last_u = curr_u;
		curr_u = v / (2 * (float) M_PI);

		for (int i = 1; i < current_points.size(); i++) {
			points.push_back(last_points[i]);
			points.push_back(last_points[i - 1]);
			points.push_back(current_points[i - 1]);
			points.push_back(last_points[i]);
			points.push_back(current_points[i - 1]);
			points.push_back(current_points[i]);

			last_v = curr_v;
			curr_v = (current_points[i].y + 1.f) / 2;

			tex.push_back(glm::vec2(last_u, curr_v));
			tex.push_back(glm::vec2(last_u, last_v));
			tex.push_back(glm::vec2(curr_u, last_v));
			tex.push_back(glm::vec2(last_u, curr_v));
			tex.push_back(glm::vec2(curr_u, last_v));
			tex.push_back(glm::vec2(curr_u, curr_v));

			//std::cout << last_v << " " << curr_v << " " << curve[i].y << std::endl;
		}
	}
}

void assign_bodies(std::vector<Planet*>& bodies, glm::vec4& origin) {
	for (int i = 0; i < bodies.size(); i++) {
		delete bodies[i];
	}
	bodies.clear();
	bodies.push_back(new Planet("textures/sun.jpg", GL_NEAREST, glm::vec4(0.f, 0.f, 0.f, 1.f), 0.22f, &origin, 1.f, 0.0, M_PI));
	bodies.push_back(new Planet("textures/earth.jpg", GL_NEAREST, glm::vec4(3.f, 0.f, 0.f, 1.f), 0.47f, &bodies[0]->curr_position, 0.2f, 0.873, 2 * M_PI));
	bodies.push_back(new Planet("textures/moon.jpg", GL_NEAREST, glm::vec4(0.3f, 0.f, 0.f, 1.f), 0.09f, &bodies[1]->curr_position, 0.02f, 3.49, M_PI * 0.5));
	bodies.push_back(new Planet("textures/stars.jpg", GL_NEAREST, glm::vec4(0.f, 0.f, 0.f, 1.f), 0.f, &origin, 10.f, 0.0, 0.0));

	std::vector<glm::vec3> sphere_points;
	std::vector<glm::vec2> tex_points;
	sphere(sphere_points, tex_points);

	for (int i = 0; i < 4; i++) {
		bodies[i]->setTransformation();

		bodies[i]->cgeom.verts = sphere_points;
		bodies[i]->cgeom.cols.resize(bodies[i]->cgeom.verts.size(), glm::vec3(0.f, 0.f, 0.f));
		bodies[i]->cgeom.tex = tex_points;
		bodies[i]->cgeom.normals = sphere_points; //Radius is 1, so vertices are also normals

		bodies[i]->ggeom.setVerts(bodies[i]->cgeom.verts);
		bodies[i]->ggeom.setCols(bodies[i]->cgeom.cols);
		bodies[i]->ggeom.setTexture(bodies[i]->cgeom.tex);
		bodies[i]->ggeom.setNormals(bodies[i]->cgeom.normals);
	}
}

void restart(std::vector<Planet*>& bodies) {
	for (int i = 0; i < 4; i++) {
		bodies[i]->orbital_angle = 0.f;
		bodies[i]->axial_rotation = 0.f;
		bodies[i]->change_ang(0.0);
	}
}
