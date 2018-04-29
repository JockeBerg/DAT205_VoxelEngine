#include "World.h"

using namespace glm;

World::World() {
	time(&_seed);
	for (int x = 0; x < WORLD::X; ++x) {
		for (int y = 0; y < WORLD::Y; ++y) {
			for (int z = 0; z < WORLD::Z; ++z) {
				_chunk[x][y][z] = new Chunk(x - WORLD::X / 2, y - WORLD::Y / 2, z - WORLD::Z / 2);
			}
		}
	}

	for (int x = 0; x < WORLD::X; ++x) {
		for (int y = 0; y < WORLD::Y; ++y) {
			for (int z = 0; z < WORLD::Z; ++z) {
				if (x > 0)
					_chunk[x][y][z]->setNeighbour(LEFT, _chunk[x - 1][y][z]);
				if (x < WORLD::X - 1)
					_chunk[x][y][z]->setNeighbour(RIGHT, _chunk[x + 1][y][z]);
				if (y > 0)
					_chunk[x][y][z]->setNeighbour(BELOW, _chunk[x][y - 1][z]);
				if (y < WORLD::Y - 1)
					_chunk[x][y][z]->setNeighbour(ABOVE, _chunk[x][y + 1][z]);
				if (z > 0)
					_chunk[x][y][z]->setNeighbour(FRONT, _chunk[x][y][z - 1]);
				if (z < WORLD::Z - 1)
					_chunk[x][y][z]->setNeighbour(BACK, _chunk[x][y][z + 1]);
			}
		}
	}
}

World::~World() {
	for (int x = 0; x < WORLD::X; ++x) {
		for (int y = 0; y < WORLD::Y; ++y) {
			for (int z = 0; z < WORLD::Z; ++z) {
				delete _chunk[x][y][z];
			}
		}
	}
}

uint8_t World::getBlock(int x, int y, int z) const {
	int cx = (x + CHUNK::X * (WORLD::X / 2)) / CHUNK::X;
	int cy = (y + CHUNK::Y * (WORLD::Y / 2)) / CHUNK::Y;
	int cz = (z + CHUNK::Z * (WORLD::Z / 2)) / CHUNK::Z;

	if (cx < 0 || cx >= WORLD::X || cy < 0 || cy >= WORLD::Y || cz <= 0 || cz >= WORLD::Z)
		return 0;

	return _chunk[cx][cy][cz]->getBlock(x & (CHUNK::X - 1), y & (CHUNK::Y - 1), z & (CHUNK::Z - 1));
}

void World::setBlock(int x, int y, int z, uint8_t type) {
	int cx = (x + CHUNK::X * (WORLD::X / 2)) / CHUNK::X;
	int cy = (y + CHUNK::Y * (WORLD::Y / 2)) / CHUNK::Y;
	int cz = (z + CHUNK::Z * (WORLD::Z / 2)) / CHUNK::Z;

	if (cx < 0 || cx >= WORLD::X || cy < 0 || cy >= WORLD::Y || cz <= 0 || cz >= WORLD::Z)
		return;

	_chunk[cx][cy][cz]->setBlock(x & (CHUNK::X - 1), y & (CHUNK::Y - 1), z & (CHUNK::Z - 1), type);
}

void World::render(const mat4 &pv) {
	float ud = 999999.0f;
	int ux = -1;
	int uy = -1;
	int uz = -1;

	for (int x = 0; x < WORLD::X; ++x) {
		for (int y = 0; y < WORLD::Y; ++y) {
			for (int z = 0; z < WORLD::Z; ++z) {
				mat4 model = translate(mat4(1.0f), vec3(_chunk[x][y][z]->getX() * CHUNK::X, _chunk[x][y][z]->getY() * CHUNK::Y, _chunk[x][y][z]->getZ() * CHUNK::Z));
				mat4 mvp = pv * model;

				// Is this chunk on the screen?
				vec4 center = mvp * vec4(CHUNK::X / 2, CHUNK::Y / 2, CHUNK::Z / 2, 1);

				float d = length(center);
				center.x /= center.w;
				center.y /= center.w;

				// If it is behind the camera, don't bother drawing it
				if (center.z < -CHUNK::Y / 2)
					continue;

				// If it is outside the screen, don't bother drawing it
				if (fabsf(center.x) > 1 + fabsf(CHUNK::Y * 2 / center.w) || fabsf(center.y) > 1 + fabsf(CHUNK::Y * 2 / center.w))
					continue;

				// If this chunk is not initialized, skip it
				if (!_chunk[x][y][z]->isInitialized()) {
					// But if it is the closest to the camera, mark it for initialization
					if (ux < 0 || d < ud) {
						ud = d;
						ux = x;
						uy = y;
						uz = z;
					}
					continue;
				}

				glUniformMatrix4fv(PROGRAM::uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

				_chunk[x][y][z]->render();
			}
		}
	}

	if (ux >= 0) {
		_chunk[ux][uy][uz]->noise(_seed);
		if (_chunk[ux][uy][uz]->getNeighbour(LEFT))
			_chunk[ux][uy][uz]->getNeighbour(LEFT)->noise(_seed);
		if (_chunk[ux][uy][uz]->getNeighbour(RIGHT))
			_chunk[ux][uy][uz]->getNeighbour(RIGHT)->noise(_seed);
		if (_chunk[ux][uy][uz]->getNeighbour(BELOW))
			_chunk[ux][uy][uz]->getNeighbour(BELOW)->noise(_seed);
		if (_chunk[ux][uy][uz]->getNeighbour(ABOVE))
			_chunk[ux][uy][uz]->getNeighbour(ABOVE)->noise(_seed);
		if (_chunk[ux][uy][uz]->getNeighbour(FRONT))
			_chunk[ux][uy][uz]->getNeighbour(FRONT)->noise(_seed);
		if (_chunk[ux][uy][uz]->getNeighbour(BACK))
			_chunk[ux][uy][uz]->getNeighbour(BACK)->noise(_seed);
		_chunk[ux][uy][uz]->initialize();
	}
}