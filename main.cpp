#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <GL/glew.h>
#include <GL/glut.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../common/shader_utils.h"
#include "textures.c"
#include "World.h"

static GLuint program;
static GLuint texture;
static GLint uniform_texture;
static GLuint cursor_vbo;

static glm::vec3 position;
static glm::vec3 forward;
static glm::vec3 right;
static glm::vec3 up;
static glm::vec3 lookat;
static glm::vec3 angle;

static int ww, wh;
static int mx, my, mz;
static int face;

static unsigned int keys;

#define M_PI 3.1415926535

static World *world;

static void update_vectors() {
	forward.x = sinf(angle.x);
	forward.y = 0;
	forward.z = cosf(angle.x);

	right.x = -cosf(angle.x);
	right.y = 0;
	right.z = sinf(angle.x);

	lookat.x = sinf(angle.x) * cosf(angle.y);
	lookat.y = sinf(angle.y);
	lookat.z = cosf(angle.x) * cosf(angle.y);

	up = glm::cross(right, lookat);
}

static int init_resources() {
	program = create_program("baseShader.vert", "baseShader.frag");

	if (program == 0)
		return 0;

	PROGRAM::attribute_coord = get_attrib(program, "coord");
	PROGRAM::uniform_mvp = get_uniform(program, "mvp");

	if (PROGRAM::attribute_coord == -1 || PROGRAM::uniform_mvp == -1)
		return 0;

	/* Create and upload the texture */

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures.width, textures.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textures.pixel_data);
	glGenerateMipmap(GL_TEXTURE_2D);


	world = new World;

	position = glm::vec3(0, CHUNK::Y + 1, 0);
	angle = glm::vec3(0, -0.5, 0);
	update_vectors();


	glGenBuffers(1, &cursor_vbo);


	glUseProgram(program);
	glUniform1i(uniform_texture, 0);
	glClearColor(0.6, 0.8, 1.0, 0.0);
	glEnable(GL_CULL_FACE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glPolygonOffset(1, 1);

	glEnableVertexAttribArray(PROGRAM::attribute_coord);

	return 1;
}

static void reshape(int w, int h) {
	ww = w;
	wh = h;
	glViewport(0, 0, w, h);
}

static float fract(float value) {
	float f = value - floorf(value);
	if (f > 0.5)
		return 1 - f;
	else
		return f;
}

static void display() {
	glm::mat4 view = glm::lookAt(position, position + lookat, up);
	glm::mat4 projection = glm::perspective(45.0f, 1.0f*ww / wh, 0.01f, 1000.0f);

	glm::mat4 mvp = projection * view;

	glUniformMatrix4fv(PROGRAM::uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);


	world->render(mvp);


	//Find block at the center of the window

	float depth;
	glReadPixels(ww / 2, wh / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	glm::vec4 viewport = glm::vec4(0, 0, ww, wh);
	glm::vec3 wincoord = glm::vec3(ww / 2, wh / 2, depth);
	glm::vec3 objcoord = glm::unProject(wincoord, view, projection, viewport);

	/* Find out which block it belongs to */

	mx = objcoord.x;
	my = objcoord.y;
	mz = objcoord.z;
	if (objcoord.x < 0)
		mx--;
	if (objcoord.y < 0)
		my--;
	if (objcoord.z < 0)
		mz--;

	/* Find out which face of the block we are looking at */

	if (fract(objcoord.x) < fract(objcoord.y))
		if (fract(objcoord.x) < fract(objcoord.z))
			face = 0; // X
		else
			face = 2; // Z
	else
		if (fract(objcoord.y) < fract(objcoord.z))
			face = 1; // Y
		else
			face = 2; // Z

	if (face == 0 && lookat.x > 0)
		face += 3;
	if (face == 1 && lookat.y > 0)
		face += 3;
	if (face == 2 && lookat.z > 0)
		face += 3;

	//Bounding box around pointer

	float bx = mx;
	float by = my;
	float bz = mz;

	float box[24][4] = {
		{ bx + 0, by + 0, bz + 0, 14 },
		{ bx + 1, by + 0, bz + 0, 14 },
		{ bx + 0, by + 1, bz + 0, 14 },
		{ bx + 1, by + 1, bz + 0, 14 },
		{ bx + 0, by + 0, bz + 1, 14 },
		{ bx + 1, by + 0, bz + 1, 14 },
		{ bx + 0, by + 1, bz + 1, 14 },
		{ bx + 1, by + 1, bz + 1, 14 },

		{ bx + 0, by + 0, bz + 0, 14 },
		{ bx + 0, by + 1, bz + 0, 14 },
		{ bx + 1, by + 0, bz + 0, 14 },
		{ bx + 1, by + 1, bz + 0, 14 },
		{ bx + 0, by + 0, bz + 1, 14 },
		{ bx + 0, by + 1, bz + 1, 14 },
		{ bx + 1, by + 0, bz + 1, 14 },
		{ bx + 1, by + 1, bz + 1, 14 },

		{ bx + 0, by + 0, bz + 0, 14 },
		{ bx + 0, by + 0, bz + 1, 14 },
		{ bx + 1, by + 0, bz + 0, 14 },
		{ bx + 1, by + 0, bz + 1, 14 },
		{ bx + 0, by + 1, bz + 0, 14 },
		{ bx + 0, by + 1, bz + 1, 14 },
		{ bx + 1, by + 1, bz + 0, 14 },
		{ bx + 1, by + 1, bz + 1, 14 },
	};

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glUniformMatrix4fv(PROGRAM::uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
	glBindBuffer(GL_ARRAY_BUFFER, cursor_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(PROGRAM::attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_LINES, 0, 24);

	glutSwapBuffers();
}

static void keyboardPress(unsigned char key, int x, int y) {
	switch (key) {
		case 'w':
		case 'W':
			keys |= 1;
			break;
		case 'a':
		case 'A':
			keys |= 2;
			break;
		case 's':
		case 'S':
			keys |= 4;
			break;
		case 'd':
		case 'D':
			keys |= 8;
			break;
		case ' ':
			keys |= 16;
			break;
		case 'c':
		case 'C':
			keys |= 32;
			break;
	}
}

static void keyboardRelease(unsigned char key, int x, int y) {
	switch (key) {
	case 'w':
	case 'W':
		keys &= ~1;
		break;
	case 'a':
	case 'A':
		keys &= ~2;
		break;
	case 's':
	case 'S':
		keys &= ~4;
		break;
	case 'd':
	case 'D':
		keys &= ~8;
		break;
	case ' ':
		keys &= ~16;
		break;
	case 'c':
	case 'C':
		keys &= ~32;
		break;
	}
}
static void specialkey(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_F1:
			exit(0);
	}
}

static void idle() {
	static int pt = 0;
	static const float movespeed = 10;

	int t = glutGet(GLUT_ELAPSED_TIME);
	float dt = (t - pt) * 1.0e-3;
	pt = t;

	if (keys & 1)
		position += forward * movespeed * dt;
	if (keys & 2)
		position -= right * movespeed * dt;
	if (keys & 4)
		position -= forward * movespeed * dt;
	if (keys & 8)
		position += right * movespeed * dt;
	if (keys & 16)
		position.y += movespeed * dt;
	if (keys & 32)
		position.y -= movespeed * dt;

	glutPostRedisplay();
}

static void motion(int x, int y) {
	static bool warp = false;
	static const float mousespeed = 0.001;

	if (!warp) {
		angle.x -= (x - ww / 2) * mousespeed;
		angle.y -= (y - wh / 2) * mousespeed;

		if (angle.x < -M_PI)
			angle.x += M_PI * 2;
		if (angle.x > M_PI)
			angle.x -= M_PI * 2;
		if (angle.y < -M_PI / 2)
			angle.y = -M_PI / 2;
		if (angle.y > M_PI / 2)
			angle.y = M_PI / 2;

		update_vectors();

		warp = true;
		glutWarpPointer(ww / 2, wh / 2);
	}
	else {
		warp = false;
	}
}

static void mouse(int button, int state, int x, int y) {
	if (state != GLUT_DOWN)
		return;

	if (button == 0) {
		if (face == 0)
			mx++;
		if (face == 3)
			mx--;
		if (face == 1)
			my++;
		if (face == 4)
			my--;
		if (face == 2)
			mz++;
		if (face == 5)
			mz--;
		world->setBlock(mx, my, mz, 6);
	}
	else {
		world->setBlock(mx, my, mz, 0);
	}
}

static void free_resources() {
	delete world;
	glDeleteProgram(program);
}

int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(WINDOW::WIDTH, WINDOW::HEIGHT);
	glutCreateWindow("Voxel Engine DAT205");

	GLenum glew_status = glewInit();
	if (GLEW_OK != glew_status) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
		return 1;
	}

	if (!GLEW_VERSION_2_0) {
		fprintf(stderr, "No support for OpenGL 2.0 found\n");
		return 1;
	}

	if (init_resources()) {
		glutSetCursor(GLUT_CURSOR_NONE);
		glutWarpPointer(WINDOW::WIDTH / 2, WINDOW::HEIGHT / 2);
		glutDisplayFunc(display);
		glutReshapeFunc(reshape);
		glutIdleFunc(display);
		glutKeyboardFunc(keyboardPress);
		glutKeyboardUpFunc(keyboardRelease);
		glutSpecialFunc(specialkey);
		glutIdleFunc(idle);
		glutPassiveMotionFunc(motion);
		glutMotionFunc(motion);
		glutMouseFunc(mouse);
		glutMainLoop();
	}
	
	atexit(free_resources);
	return 0;
}