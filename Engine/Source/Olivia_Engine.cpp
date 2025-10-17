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
		// Pipelines
		PipelineHandle opaque_pipeline = m_renderer->get_pipeline(PIPELINE_OPAQUE);

		// Registry
		entt::registry registry{};
		registry.ctx().emplace<InputState>();
		// Create entity
		auto entity = registry.create();

		// Assign components
		registry.emplace<CMesh>(entity, MESH_QUAD);
		registry.emplace<CPosition>(entity, 0.0f, 0.0f, 0.0f);
		registry.emplace<CRotation>(entity, 0.0f, 0.0f, 0.0f);
		registry.emplace<CScale>(entity, 0.0f, 0.0f, 0.0f);
		
		SDL_Event event;

		while (m_running)
		{
			while (SDL_PollEvent(&event))
			{
				if(event.type == SDL_EVENT_QUIT)
					m_running = false;

				InputSystem(registry, event);
			}

			MovementSystem(registry, 0.016f);

			m_renderer->begin_frame();

			RenderSystem(registry);

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