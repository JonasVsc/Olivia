#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <glm/glm.hpp>

namespace Olivia
{
	struct Window;
	struct Allocator;
	
	constexpr uint32_t MAX_CONCURRENT_FRAMES{ 2 };
	constexpr uint32_t MAX_INSTANCES{ 10000 };
	constexpr uint32_t MAX_MESHES{ 100 };

	// CONTEXT

	struct RendererContext
	{
		void* window_handle;
		bool vsync;
		VkInstance instance;
		VkSurfaceKHR surface;
		VkPhysicalDevice gpu;
		VkQueue queue;
		uint32_t graphics_queue_index;
		VkDevice device;
		VmaAllocator allocator;
		VkCommandPool command_pool;
		VkCommandBuffer command_buffers[5];
		VkSwapchainKHR swapchain;
		uint32_t swapchain_size;
		VkExtent2D swapchain_extent;
		VkImage swapchain_images[5];
		VkImageView swapchain_views[5];
		VkSurfaceFormatKHR swapchain_format;
		VkFence queue_submit[MAX_CONCURRENT_FRAMES];
		VkSemaphore acquire_image[MAX_CONCURRENT_FRAMES];
		VkSemaphore present_image[5];
		uint32_t current_frame;
		uint32_t image_index;
	};

	// HELPERS

	struct Buffer
	{
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo info;
	};

	struct Vertex
	{
		float position[3];
		float normal[3];
		float color[4];
		float uv[2];
		uint32_t texture;
	};

	struct InstanceData
	{
		glm::vec4 model_matrix_row0;
		glm::vec4 model_matrix_row1;
		glm::vec4 model_matrix_row2;
		glm::vec4 model_matrix_row3;
		glm::vec4 color;
		uint32_t mesh;
	};

	struct InstanceData
	{
		glm::vec4 model_matrix_row0;
		glm::vec4 model_matrix_row1;
		glm::vec4 model_matrix_row2;
		glm::vec4 model_matrix_row3;
		glm::vec4 color;
		uint32_t mesh;
	};

	// MANAGERS

	struct AssetManager
	{
		Buffer mesh_buffer;
		uint32_t mesh_count;
		
		Vertex vertices[2'000'000];
		uint32_t indices[2'000'000];

		VkDeviceSize vertex_offset[MAX_MESHES];
		VkDeviceSize index_offset[MAX_MESHES];
		uint32_t index_count[MAX_MESHES];

		VkDeviceSize total_vertex_bytes;
		VkDeviceSize total_index_bytes;
	};

	// RENDERER

	struct Renderer
	{
		RendererContext ctx;

		AssetManager assets_manager;
	};

	Renderer* GetRenderer();

	Renderer* CreateRenderer(Allocator* allocator, Window* window);

	void DestroyRenderer(Renderer* r);

	void Draw(Renderer* r);

} // Olivia