#pragma once
#include "olivia/olivia_core.h"
#include <SDL3/SDL.h>

namespace olivia
{
	SDL_Window* init_window(const char* title, int width, int height);

	void destroy_window(SDL_Window* window);
}
