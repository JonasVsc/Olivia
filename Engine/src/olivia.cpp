#include "olivia/olivia.h"

namespace olivia
{

	void run()
	{
		SDL_Window* window = SDL_CreateWindow("Olivia", 640, 480, SDL_WINDOW_VULKAN);

		if (!window)
			return;

		init_vulkan_core(window);

		bool running{ true };
		while (running)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_EVENT_QUIT)
					running = false;
			}

			begin_frame();

			end_frame();
		}

		destroy_vulkan_core();

		SDL_DestroyWindow(window);
	}

} // olivia

int main(int argc, char* argv[])
{
	olivia::run();

	return 0;
}