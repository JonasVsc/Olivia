#pragma once
#include "olivia/olivia_context.h"
#include "olivia/olivia_platform.h"
#include "olivia/olivia_renderer.h"
#include "olivia/olivia_graphics.h"
#include "olivia/olivia_runtime.h"

namespace olivia
{
	struct context_t
	{
		SDL_Window* window;
		renderer    renderer;
		bool        running;
	};

	extern context g_context;

	void quit();
}