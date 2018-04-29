#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace WINDOW {
	static const int WIDTH = 1024;
	static const int HEIGHT = 768;
}

namespace PROGRAM {
	static GLint attribute_coord;
	static GLint uniform_mvp;
}

namespace BLOCK {
	static const int TRANSPARENCY[16] = { 2, 0, 0, 0, 1, 0, 0, 0, 3, 4, 0, 0, 0, 0, 0, 0 };
	static const char *NAMES[16] = {
		"air", "dirt", "topsoil", "grass", "leaves", "wood", "stone", "sand",
		"water", "glass", "brick", "ore", "woodrings", "white", "black", "x-y"
	};
}

namespace CHUNK {
	static const int X = 16;
	static const int Y = 16;
	static const int Z = 16;
}

namespace WORLD {
	static const int SEALEVEL = 4;
	static const int X = 8;
	static const int Y = 8;
	static const int Z = 8;
}