#pragma once
#include "olivia_core.h"
#include "olivia_platform.h"

namespace olivia
{
	OLIVIA_DEFINE_HANDLE(renderer);

	renderer init_renderer(lifetime_allocator allocator, SDL_Window* window);

	void begin_frame(renderer renderer);

	void end_frame(renderer renderer);

}