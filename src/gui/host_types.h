#ifndef HOST_TYPES_H
#define HOST_TYPES_H

#include <cstdint>

struct VideoUpdate {
	const uint32_t *buffer = nullptr;
	int xsize = 0;
	int ysize = 0;
	int yl = 0;
	int yh = 0;
	int double_size = 0;
	int host_xsize = 0;
	int host_ysize = 0;
};

struct MouseMoveUpdate {
	int16_t x = 0;
	int16_t y = 0;
};

#endif
