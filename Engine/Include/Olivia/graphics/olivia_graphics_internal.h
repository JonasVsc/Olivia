#pragma once
#include "olivia/olivia_graphics.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace olivia
{
	constexpr size_t   MESH_ATLAS_V_SIZE{ MEGABYTES(128) };
	constexpr size_t   MESH_ATLAS_I_SIZE{ MEGABYTES(128) };
	constexpr uint32_t MESHES_PER_ATLAS{ 10 };
	constexpr uint32_t MESH_ATLAS_COUNT{ 10 };
	constexpr uint32_t MAX_MESHES{ 10'000 };
	constexpr uint32_t MAX_PIPELINES{ 100 };
	constexpr uint32_t MAX_INSTANCES{ 10'000 };

	OLIVIA_DEFINE_HANDLE(graphics_allocator);

	struct buffer_t
	{
		VkBuffer          buffer;
		VmaAllocation     allocation;
		VmaAllocationInfo info;
	};

	struct vertex_t
	{
		float position[3];
		float normal[3];
		float uv[2];
	};

	struct instance_t
	{
		float transform[16];
	};

	struct mesh_atlas_info_t
	{
		bool initialized;

		uint32_t v_used_space;
		uint32_t i_used_space;
	};

	struct mesh_atlas_t
	{
		bool initialized;

		buffer_t v_buffer;
		buffer_t i_buffer;

		uint32_t i_offset[MAX_MESHES];
		uint32_t v_offset[MAX_MESHES];
		uint32_t i_count[MAX_MESHES];
	};

	struct graphics_allocator_t
	{
		VkDevice     device;
		VmaAllocator allocator;
		VkFormat     format;

		VkPipeline       pipelines[MAX_PIPELINES];
		VkPipelineLayout layouts[MAX_PIPELINES];
		uint32_t         pipeline_count;

		buffer_t         instance_buffer;

		mesh_atlas_info_t mesh_atlas_info[MESH_ATLAS_COUNT];
		mesh_atlas_t      mesh_atlas[MESH_ATLAS_COUNT];
		uint32_t          mesh_count;

		// ready-to-use meshes
		mesh_t triangle_mesh;
		mesh_t quad_mesh;
		mesh_t cube_mesh;
	};

	extern graphics_allocator local_graphics_allocator;

	mesh_atlas_t& request_graphics_allocator(size_t vertices_size, size_t indices_size);

	buffer_t create_buffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size);
	
	VkShaderModule create_shader_module(VkDevice device, const char* shader_path);

	// ready-to-use meshes

	void create_quad_mesh();

	void create_cube_mesh();


} // olivia