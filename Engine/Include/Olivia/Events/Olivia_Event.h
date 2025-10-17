#pragma once
#include "Olivia/Olivia_Core.h"

namespace Olivia
{
	struct InputState
	{
		bool current_keys[SDL_SCANCODE_COUNT]{};
		bool previous_keys[SDL_SCANCODE_COUNT]{};
	};

	extern InputState g_input_state;

	inline bool is_key_pressed(SDL_Scancode code)
	{
		return (code >= 0 && code < 256) ? (g_input_state.current_keys[code] && !g_input_state.previous_keys[code]) : false;
	}

	inline bool is_key_down(SDL_Scancode code)
	{
		return (code >= 0 && code < 256) ? g_input_state.current_keys[code] : false;
	}

	inline void update_input_state()
	{
		memcpy(g_input_state.previous_keys, g_input_state.current_keys, sizeof(g_input_state.current_keys));
	}
}