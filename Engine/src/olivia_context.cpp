#include "olivia/context/olivia_context_internal.h"

namespace olivia
{
	static lifetime_allocator local_lifetime_allocator{ nullptr };

	context g_context{ nullptr };

	bool init(const char* game_dll)
	{
		// init life time memory for context storage

		local_lifetime_allocator = create_lifetime_allocator(MEGABYTES(5));

		g_context = (context)allocate(local_lifetime_allocator, sizeof(context_t));

		// init game_dll and query it config

		struct config
		{
			const char* window_title = "Olivia";
			int32_t window_width     = 640;
			int32_t window_height    = 480;
		} config;

		// init window

		g_context->window = init_window(config.window_title, config.window_width, config.window_height);

		if (!g_context->window)
		{
			LOG_ERROR(TAG_PLATFORM, "failed to create window");
			return false;
		}

		// init renderer

		g_context->renderer = init_renderer(local_lifetime_allocator, g_context->window);

		if (!g_context->renderer)
		{
			LOG_ERROR(TAG_PLATFORM, "failed to create renderer");
			return false;
		}

		// init graphics

		init_graphics_allocator(local_lifetime_allocator, g_context->renderer);

		// load registry

		registry reg0 = create_registry();

		entity_t ent0 = create_entity(reg0, { 0, 
			{ 0.0f, 0.0f, 0.0f }, 
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f } 
		});

		entity_t ent1 = create_entity(reg0, { 0,
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f }
			});

		entity_t ent2 = create_entity(reg0, { 0,
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f }
			});

		entity_t ent3 = create_entity(reg0, { 0,
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f }
			});

		entity_t ent4 = create_entity(reg0, { 1,
			{ 0.0f, 3.0f, 0.0f },
			{ 6.0f, 0.0f,-2.0f },
			{ 1.0f, 1.0f, 1.0f }
		});

		entity_t ent5 = create_entity(reg0, { 0,
			{ 0.0f,-2.0f, 0.0f },
			{ 0.0f, 0.0f, 2.0f },
			{ 1.0f, 1.0f, 1.0f }
			});

		entity_t ent6 = create_entity(reg0, { 1,
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f },
			{ 1.0f, 1.0f, 3.0f }
		});

		bind_registry(reg0);

		pipeline_t pipeline = create_graphics_pipeline({ "olivia.vert.spv", "olivia.frag.spv" });

		g_context->running = true;
		return true;
	}

	void run()
	{
		while (g_context->running)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_EVENT_QUIT)
				{
					g_context->running = false;
				}
			}

			// register game systems

			begin_frame(g_context->renderer);

			render_system();

			end_frame(g_context->renderer);
		}

		quit();
	}

	void quit()
	{
		destroy_lifetime_allocator(local_lifetime_allocator);
	}
}