#pragma once
#include "Olivia/Olivia_Events.h"

namespace Olivia
{
	class InputSystem
	{
	public:

		static inline void update(SDL_Event& event)
		{
			if (event.type == SDL_EVENT_KEY_DOWN)
				g_input_state.current_keys[event.key.scancode] = true;
			if (event.type == SDL_EVENT_KEY_UP)
				g_input_state.current_keys[event.key.scancode] = false;
		}
	};
}