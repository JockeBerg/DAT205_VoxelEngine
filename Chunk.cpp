#include "Chunk.h"

using namespace glm;
typedef tvec4<GLbyte, mediump> byte4;

Chunk::Chunk(int x, int y, int z) : _x(x), _y(y), _z(z) {
	memset(_block, 0, sizeof(_block));
	_front = _back = _above = _below = _left = _right = 0;
	_slot = 0;
	_changed = true;
	_initialized = false;
	_noised = false;
	glGenBuffers(1, &_vbo);
}


Chunk::~Chunk() {
	glDeleteBuffers(1, &_vbo);
}



uint8_t Chunk::getBlock(int x, int y, int z) const {
	if (x < 0)
		return _left ? _left->_block[x + CHUNK::X][y][z] : 0;
	if (x >= CHUNK::X)
		return _right ? _right->_block[x - CHUNK::X][y][z] : 0;
	if (y < 0)
		return _below ? _below->_block[x][y + CHUNK::Y][z] : 0;
	if (y >= CHUNK::Y)
		return _above ? _above->_block[x][y - CHUNK::Y][z] : 0;
	if (z < 0)
		return _front ? _front->_block[x][y][z + CHUNK::Z] : 0;
	if (z >= CHUNK::Z)
		return _back ? _back->_block[x][y][z - CHUNK::Z] : 0;
	return _block[x][y][z];
}

void Chunk::setBlock(int x, int y, int z, uint8_t type) {
	// If coordinates are outside this chunk, find the right one.
	if (x < 0) {
		if (_left)
			_left->setBlock(x + CHUNK::X, y, z, type);
		return;
	}
	if (x >= CHUNK::X) {
		if (_right)
			_right->setBlock(x - CHUNK::X, y, z, type);
		return;
	}
	if (y < 0) {
		if (_below)
			_below->setBlock(x, y + CHUNK::Y, z, type);
		return;
	}
	if (y >= CHUNK::Y) {
		if (_above)
			_above->setBlock(x, y - CHUNK::Y, z, type);
		return;
	}
	if (z < 0) {
		if (_front)
			_front->setBlock(x, y, z + CHUNK::Z, type);
		return;
	}
	if (z >= CHUNK::Z) {
		if (_back)
			_back->setBlock(x, y, z - CHUNK::Z, type);
		return;
	}

	// Change the block
	_block[x][y][z] = type;
	_changed = true;

	// When updating blocks at the edge of this chunk,
	// visibility of blocks in the neighbouring chunk might change.
	if (x == 0 && _left)
		_left->_changed = true;
	if (x == CHUNK::X - 1 && _right)
		_right->_changed = true;
	if (y == 0 && _below)
		_below->_changed = true;
	if (y == CHUNK::Y - 1 && _above)
		_above->_changed = true;
	if (z == 0 && _front)
		_front->_changed = true;
	if (z == CHUNK::Z - 1 && _back)
		_back->_changed = true;
}

float Chunk::noise2d(int octaves, float x, float y, int seed) {
	float sum = 0;
	float scale = 1.0;

	for (int i = 0; i < octaves; ++i) {
		sum += simplex(vec2(x, y) * scale);
		scale *= 2.0;
	}

	return sum;
}

float Chunk::noise3d(int octaves, float x, float y, float z, int seed) {
	float sum = 0;
	float scale = 1.0;

	for (int i = 0; i < octaves; ++i) {
		sum += simplex(vec3(x, y, z) * scale);
		scale *= 2.0;
	}

	return sum;
}

void Chunk::noise(int seed) {
	if (_noised)
		return;
	else
		_noised = true;

	for (int x = 0; x < CHUNK::X; ++x) {
		for (int z = 0; z < CHUNK::Z; ++z) {
			// Land height
			float n = noise2d(6, (x + _x * CHUNK::X) / 256.0, (z + _z * CHUNK::Z) / 256.0, seed) * WORLD::SEALEVEL;
			int h = n * 2;
			int y = 0;

			// Land blocks
			for (y = 0; y < CHUNK::Y; ++y) {
				// Are we above "ground" level?
				if (y + _y * CHUNK::Y >= h) {
					// If we are not yet up to sea level, fill with water blocks
					if (y + _y * CHUNK::Y < WORLD::SEALEVEL) {
						_block[x][y][z] = 8;
						continue;
					}
					else {
						break;
					}
				}

				float r = noise3d(3, (x + _x * CHUNK::X) / 16.0, (y + _y * CHUNK::Y) / 16.0, (z + _z * CHUNK::Z) / 16.0, seed);

				if (n + r * 5 < 2 * WORLD::SEALEVEL)
					_block[x][y][z] = (h < WORLD::SEALEVEL || y + _y * CHUNK::Y < h - 1) ? 1 : 3;
				else
					_block[x][y][z] = 6;
			}
		}
	}
	_changed = true;
}

void Chunk::update() {
	byte4 vertex[CHUNK::X * CHUNK::Y * CHUNK::Z * 18];
	int i = 0;
	for (int x = 0; x < CHUNK::X; ++x) {
		for (int y = 0; y < CHUNK::Y; ++y) {
			for (int z = 0; z < CHUNK::Z; ++z) {
				uint8_t type = _block[x][y][z];
				if (type != 0) {

					bool add = !(z == 0 && _z == -WORLD::Z / 2);
					//Check X min boundaries, add front faces
					if (!add || (add && !getBlock(x, y, z - 1))) {
						vertex[i++] = byte4(x, y, z, type);
						vertex[i++] = byte4(x + 1, y, z, type);
						vertex[i++] = byte4(x + 1, y + 1, z, type);
						vertex[i++] = byte4(x + 1, y + 1, z, type);
						vertex[i++] = byte4(x, y + 1, z, type);
						vertex[i++] = byte4(x, y, z, type);
					}

					add = !(z == CHUNK::Z - 1 && _z == WORLD::Z / 2 - 1);

					//Check X max boundaries, add back faces
					if (!add || (add && !getBlock(x, y, z + 1))) {
						vertex[i++] = byte4(x, y, z + 1, type);
						vertex[i++] = byte4(x + 1, y, z + 1, type);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, type);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, type);
						vertex[i++] = byte4(x, y + 1, z + 1, type);
						vertex[i++] = byte4(x, y, z + 1, type);
					}

					add = !(x == 0 && _x == -WORLD::X / 2);

					//Check Z min boundaries, add left faces
					if (!add || (add && !getBlock(x - 1, y, z))) {
						vertex[i++] = byte4(x, y, z, type);
						vertex[i++] = byte4(x, y, z + 1, type);
						vertex[i++] = byte4(x, y + 1, z + 1, type);
						vertex[i++] = byte4(x, y + 1, z + 1, type);
						vertex[i++] = byte4(x, y + 1, z, type);
						vertex[i++] = byte4(x, y, z, type);
					}

					add = !(x == CHUNK::X - 1 && _x == WORLD::X / 2 - 1);

					//Check Z max boundaries, add right faces
					if (!add || (add && !getBlock(x + 1, y, z))) {
						vertex[i++] = byte4(x + 1, y, z, type);
						vertex[i++] = byte4(x + 1, y, z + 1, type);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, type);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, type);
						vertex[i++] = byte4(x + 1, y + 1, z, type);
						vertex[i++] = byte4(x + 1, y, z, type);
					}

					add = !(y == 0 && _y == -WORLD::Y / 2);

					//Check Y min boundaries, add bottom faces
					if (!add || (add && !getBlock(x, y - 1, z))) {
						vertex[i++] = byte4(x, y, z, type + 128);
						vertex[i++] = byte4(x + 1, y, z, type + 128);
						vertex[i++] = byte4(x + 1, y, z + 1, type + 128);
						vertex[i++] = byte4(x + 1, y, z + 1, type + 128);
						vertex[i++] = byte4(x, y, z + 1, type + 128);
						vertex[i++] = byte4(x, y, z, type + 128);
					}

					add = !(y == CHUNK::Y - 1 && _y == WORLD::Y / 2 - 1);

					//Check Y max boundaries, add top faces
					if (!add || (add && !getBlock(x, y + 1, z))) {
						vertex[i++] = byte4(x, y + 1, z, type + 128);
						vertex[i++] = byte4(x + 1, y + 1, z, type + 128);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, type + 128);
						vertex[i++] = byte4(x + 1, y + 1, z + 1, type + 128);
						vertex[i++] = byte4(x, y + 1, z + 1, type + 128);
						vertex[i++] = byte4(x, y + 1, z, type + 128);
					}
				}
			}
		}
	}

	_changed = false;
	_blocks = i;
	
	// If this chunk is empty, no need to allocate a chunk slot.
	if (!_blocks)
		return;
	
	// Upload vertices
	
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, i * sizeof *vertex, vertex, GL_STATIC_DRAW);
}

void Chunk::render() {
	if (_changed)
		update();

	if (!_blocks)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glVertexAttribPointer(PROGRAM::attribute_coord, 4, GL_BYTE, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLES, 0, _blocks);
}

void Chunk::setNeighbour(Orientation orientation, Chunk *neighbour) {
	switch (orientation) {
		case FRONT:
			_front = neighbour;
			break;
		case BACK:
			_back = neighbour;
			break;
		case ABOVE:
			_above = neighbour;
			break;
		case BELOW:
			_below = neighbour;
			break;
		case LEFT:
			_left = neighbour;
			break;
		case RIGHT:
			_right = neighbour;
			break;
	}
}

int Chunk::getX() {
	return _x;
}

int Chunk::getY() {
	return _y;
}

int Chunk::getZ() {
	return _z;
}

bool Chunk::isInitialized() {
	return _initialized;
}

Chunk* Chunk::getNeighbour(Orientation orientation) {
	switch (orientation) {
		case FRONT:
			return _front;
		case BACK:
			return _back;
		case ABOVE:
			return _above;
		case BELOW:
			return _below;
		case LEFT:
			return _left;
		case RIGHT:
			return _right;
	}

	return 0;
}

void Chunk::initialize() {
	_initialized = true;
}