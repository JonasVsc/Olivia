#include "olivia/platform/olivia_platform_internal.h"

namespace olivia
{
	SDL_Window* init_window(const char* title, int width, int height)
	{
		SDL_Window* result_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);

		if (result_window)
		{
			SDL_ShowWindow(result_window);
		}

		return result_window;
	}

	void destroy_window(SDL_Window* window)
	{
		if (window)
		{
			SDL_DestroyWindow(window);
		}
	}
}