#pragma once
#include "Olivia_Renderer.h"
#include "Olivia_Platform.h"

#define VK_CHECK(x)														\
	do																	\
	{																	\
		VkResult err = x;												\
		if (err)														\
		{																\
			printf("Detected Vulkan error: %d", err);					\
			abort();													\
		}																\
	} while (0)

namespace Olivia
{
	constexpr uint32_t MAX_CONCURRENT_FRAMES{ 2 };
	constexpr uint32_t MESH_ATLAS_V_BUFFER_SIZE{ MEGABYTES(100) };
	constexpr uint32_t MESH_ATLAS_I_BUFFER_SIZE{ MEGABYTES(100) };
	constexpr uint32_t MAX_MESHES{ 10 };

	enum Pipelines
	{
		OPAQUE_PIPELINE,
		PIPELINE_COUNT
	};

	enum Meshes
	{
		QUAD_MESH,
		MESH_COUNT
	};

	// CORE

	struct VulkanCore
	{
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

		VkFence imm_fence;
		VkCommandBuffer imm_command_buffer;
	};

	// LAYOUTS

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct Instance
	{
		float transform[16];
	};

	// RESOURCES

	struct Buffer
	{
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo info;
	};

	struct MeshAtlas
	{
		Buffer vertex_buffer;
		Buffer index_buffer;
		uint32_t mesh_count;

		uint32_t v_offset[MAX_MESHES];
		uint32_t v_count[MAX_MESHES];
		uint32_t i_offset[MAX_MESHES];
		uint32_t i_count[MAX_MESHES];
	};

	struct GraphicsPipeline
	{
		VkPipelineLayout layout;
		VkPipeline pipeline;
	};

	// RENDERER

	struct Renderer
	{
		VulkanCore core;
		MeshAtlas mesh_atlas;
		GraphicsPipeline pipelines[PIPELINE_COUNT];
	};

	// RENDERER INIT

	void CreateDevice();
	void CreateSwapchain();
	void CreateSyncObjects();
	void CreateMeshAtlasBuffer();
	void CreatePipelines();

	// HEPERS 

	void RecreateSwapchain();
	Buffer CreateBuffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size);
	VkShaderModule CreateShaderModule(VkDevice device, const char* shader_path);
}

