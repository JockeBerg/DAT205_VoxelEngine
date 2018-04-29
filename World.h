#pragma once

#include <time.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Constants.h"
#include "Chunk.h"

class World
{
public:
	World();
	~World();
	uint8_t getBlock(int x, int y, int z) const;
	void setBlock(int x, int y, int z, uint8_t type);
	void render(const glm::mat4 &pv);
private:
	Chunk *_chunk[WORLD::X][WORLD::Y][WORLD::Z];
	time_t _seed;
};

