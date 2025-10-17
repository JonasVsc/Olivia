#include "Olivia/Olivia_Engine.h"

namespace Olivia
{
	Engine::Engine(const char* title, int32_t width, int32_t height)
		: m_running{true}
	{
		m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);
		if (m_window)
		{
			SDL_ShowWindow(m_window);
			m_renderer = new Renderer(*m_window);
		}
	}

	Engine::~Engine()
	{
		delete m_renderer;
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}

	void Engine::run()
	{
		while (m_running)
		{
			update_input_state();

			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if(event.type == SDL_EVENT_QUIT)
					m_running = false;

				InputSystem::update(event);
			}

			m_renderer->begin_frame();

			m_renderer->end_frame();
		}
	}

} // Olivia

int main(int argc, char* argv[])
{
	Olivia::Engine eng("Olivia Engine", 640, 480);

	try
	{
		eng.run();
	}
	catch (std::exception& e)
	{
		printf("%s\n", e.what());
	}

	return 0;
}