#pragma once
#include "olivia_core.h"
#include "olivia_platform.h"
#include "olivia_graphics.h"

namespace olivia
{
	constexpr const char* OLIVIA_GAME = "olivia_game.dll";

	typedef void (*olivia_load)(void*);
	typedef void (*olivia_update)(float);
	typedef void (*olivia_draw)(void);

	struct context_t
	{
		// --- state ---

		bool running;

		// --- game ---

		const char*       olivia_game_path;
		void*             olivia_storage;
		SDL_SharedObject* olivia_game;
		olivia_update     olivia_update;
		olivia_draw       olivia_draw;
	};

	void game_load(const char* game, size_t game_storage_size);

	void reload();

	void run(const char* game);


} // olivia