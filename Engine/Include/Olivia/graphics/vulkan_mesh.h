#pragma once
#include "vulkan_buffer.h"

namespace olivia
{
	using mesh_t = uint32_t;

	constexpr size_t MESH_GROUP_V_BUFFER_SIZE{ MEGABYTES(150) };
	constexpr size_t MESH_GROUP_I_BUFFER_SIZE{ MEGABYTES(50)  };

	constexpr uint32_t MAX_MESHES{ 10 };

	struct vertex3d_t
	{
		vec3_t position;
		vec3_t normal;
		vec2_t uv;
	};

	struct mesh_group_t
	{
		vulkan_buffer_t vertex_buffer;
		vulkan_buffer_t index_buffer;
		size_t		    v_bytes_used;
		size_t          i_bytes_used;
		uint32_t        mesh_count;

		uint32_t v_offset[MAX_MESHES];
		uint32_t i_offset[MAX_MESHES];
		uint32_t v_count[MAX_MESHES];
		uint32_t i_count[MAX_MESHES];
	};

	void init_mesh_group();

	void destroy_mesh_group();

	mesh_t upload_mesh(const void* vertices, uint32_t vertex_count, const void* indices, uint32_t index_count);

} // olivia
