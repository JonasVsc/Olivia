#pragma once
#include "olivia_core.h"
#include "olivia_renderer.h"

#include <vulkan/vulkan.h>

namespace olivia
{
	using mesh_t = uint32_t;
	using pipeline_t = uint32_t;

	struct pipeline_info_t
	{
		const char* v_shader;
		const char* f_shader;
		uint32_t               set_layout_count;
		VkDescriptorSetLayout* set_layouts;
	};

	bool init_graphics_allocator(lifetime_allocator allocator, renderer renderer);

	void destroy_graphics_allocator();

	pipeline_t create_graphics_pipeline(const pipeline_info_t& info);

	inline mesh_t get_quad_mesh() { return 0; }

	inline mesh_t get_cube_mesh() { return 1; }

} // olivia