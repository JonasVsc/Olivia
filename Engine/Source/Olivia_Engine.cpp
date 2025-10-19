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

			if (m_renderer)
			{
				m_registry = new Registry(*m_renderer);
			}
		}

	}

	Engine::~Engine()
	{
		delete m_registry;
		delete m_renderer;
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}

	void Engine::run()
	{
		RenderSystem render_system(*m_registry, *m_renderer);
		MovementSystem movement_system(*m_registry);

		EntityID entity = m_registry->create({
				.position = { 0.0f, 0.0f, 0.0f },
				.rotation = { 0.0f, 0.0f, 0.0f },
				.scale    = { 0.2f, 0.2f, 1.0f },
				.mesh     = MESH_QUAD,
				.is_renderable = true
		});

		entity = m_registry->create({
				.position = { 0.5f, 0.4f, 0.0f },
				.rotation = { 0.0f, 0.0f, 0.0f },
				.scale = { 0.2f, 0.2f, 1.0f },
				.mesh = MESH_QUAD,
				.is_renderable = true,
				.is_player = true
			});

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

			movement_system.update();

			m_renderer->begin_frame();

			render_system.draw();

			m_renderer->end_frame();
		}
	}

} // Olivia