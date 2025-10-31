#pragma once
#include "graphics/vulkan_mesh.h"

namespace olivia
{
	struct renderer_t
	{
		mesh_group_t mesh_group;
	};

	void init_renderer(SDL_Window* window);

	void destroy_renderer();

} // olivia
