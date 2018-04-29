#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <cstring>
#include "Constants.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

static enum Orientation {FRONT, BACK, ABOVE, BELOW, LEFT, RIGHT};

class Chunk {
public:
	Chunk(int x, int y, int z);
	~Chunk();

	uint8_t getBlock(int x, int y, int z) const;
	void setBlock(int x, int y, int z, uint8_t type);
	static float noise2d(int octaves, float x, float y, int seed);
	static float noise3d(int octaves, float x, float y, float z, int seed);
	void noise(int seed);
	void update();
	void render();
	void setNeighbour(Orientation orientation, Chunk *neighbour);
	int getX();
	int getY();
	int getZ();
	bool isInitialized();
	void initialize();
	Chunk* getNeighbour(Orientation oriantation);


private:
	uint8_t _block[CHUNK::X][CHUNK::Y][CHUNK::Z];
	Chunk *_front, *_back, *_above, *_below, *_left, *_right;
	int _slot;
	GLuint _vbo;
	int _blocks;
	bool _changed, _noised, _initialized;
	int _x, _y, _z;
};

