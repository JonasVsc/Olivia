#pragma once
#include "olivia/olivia_core.h"

#include <SDL3/SDL.h>

namespace olivia
{
	struct input_state_t
	{
		bool curr_keys[SDL_SCANCODE_COUNT];
		bool prev_keys[SDL_SCANCODE_COUNT];
	};

	extern input_state_t g_input_state;

	void update_input_state();

	inline bool is_key_pressed(SDL_Scancode code)
	{
		return (code >= 0 && code < 256) ? (g_input_state.curr_keys[code] && !g_input_state.prev_keys[code]) : false;
	}

	inline bool is_key_down(SDL_Scancode code)
	{
		return (code >= 0 && code < 256) ? g_input_state.curr_keys[code] : false;
	}

} // olivia

