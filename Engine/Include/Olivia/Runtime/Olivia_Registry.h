#pragma once
#include "Olivia/Olivia_Core.h"
#include "Olivia_Component.h"

#include "entt/entity/registry.hpp"

namespace Olivia
{
	struct InputState
	{
		bool keys[SDL_SCANCODE_COUNT] = { false }; // keycode -> pressed
	};

} // Olivia