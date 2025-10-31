#include "olivia/olivia.h"

namespace olivia
{
	static context_t ctx{};

	void game_load(const char* game, size_t game_storage_size)
	{
		ctx.olivia_game_path = game;

		if (SDL_CopyFile(ctx.olivia_game_path, OLIVIA_GAME))
		{
			ctx.olivia_game = SDL_LoadObject(OLIVIA_GAME);

			if (ctx.olivia_game)
			{
				olivia_load load = (olivia_load)SDL_LoadFunction(ctx.olivia_game, "olivia_load");
				assert(load && "failed to get olivia_load function address");

				ctx.olivia_storage = calloc(1, game_storage_size);

				if (ctx.olivia_storage)
				{
					load(ctx.olivia_storage);

					ctx.olivia_update = (olivia_update)SDL_LoadFunction(ctx.olivia_game, "olivia_update");
					assert(ctx.olivia_update && "failed to get olivia_update function address");

					ctx.olivia_draw = (olivia_draw)SDL_LoadFunction(ctx.olivia_game, "olivia_draw");
					assert(ctx.olivia_draw && "failed to get olivia_draw function address");

					ctx.running = true;
				}
			}
		}
	}

	void reload()
	{
		SDL_UnloadObject(ctx.olivia_game);

		if (SDL_CopyFile(ctx.olivia_game_path, OLIVIA_GAME))
		{
			ctx.olivia_game = SDL_LoadObject(OLIVIA_GAME);

			if (ctx.olivia_game)
			{
				olivia_load load = (olivia_load)SDL_LoadFunction(ctx.olivia_game, "olivia_load");
				assert(load && "failed to get olivia_load function address");

				load(ctx.olivia_storage);

				ctx.olivia_update = (olivia_update)SDL_LoadFunction(ctx.olivia_game, "olivia_update");
				assert(ctx.olivia_update && "failed to get olivia_update function address");
				
				ctx.olivia_draw = (olivia_draw)SDL_LoadFunction(ctx.olivia_game, "olivia_draw");
				assert(ctx.olivia_draw && "failed to get olivia_draw function address");

				return;
			}
		}

		ctx.running = false;
	}

	void run(const char* game)
	{
		SDL_Window* window = SDL_CreateWindow("Olivia", 640, 480, SDL_WINDOW_VULKAN);
		if (!window)
		{
			LOG_ERROR(TAG_OLIVIA, "failed to create window");
			return;
		}

		init_renderer(window);

		game_load(game, MEGABYTES(200));
		
		while (ctx.running)
		{
			update_input_state();

			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_EVENT_QUIT)
					ctx.running = false;
				if (event.type == SDL_EVENT_KEY_DOWN)
					g_input_state.curr_keys[event.key.scancode] = true;
				if (event.type == SDL_EVENT_KEY_UP)
					g_input_state.curr_keys[event.key.scancode] = false;
			}

			if (is_key_pressed(SDL_SCANCODE_F5)) reload();

			ctx.olivia_update(0.016f);

			begin_frame();

			ctx.olivia_draw();
			
			end_frame();
		}

		destroy_renderer();
		SDL_DestroyWindow(window);
	}

} // olivia

int main(int argc, char* argv[])
{
	olivia::run("000_setup.dll");

	return 0;
}