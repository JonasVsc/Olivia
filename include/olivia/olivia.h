#pragma once
#include "olivia_types.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <cstdio>


#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define KILOBYTES(x) ((x) * 1024ull)
#define MEGABYTES(x) (KILOBYTES(x) * 1024ull)
#define GIGABYTES(x) (MEGABYTES(x) * 1024ull)

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


constexpr uint32_t MAX_FRAMES{ 2 };

enum MeshType
{
	MESH_MAX
};

enum PipelineType
{
	PIPELINE_2D,
	PIPELINE_3D,
	PIPELINE_MAX
};

struct context_t
{
	// --- platform ---

	SDL_Window*        window;
		
	// --- vulkan core ---

	VkInstance         instance;
	VkSurfaceKHR       surface;
	VkPhysicalDevice   gpu;
	VkQueue            queue;
	uint32_t           graphics_queue_index;
	VkDevice           device;
	VkCommandPool      command_pool;
	VkCommandBuffer    command_buffers[5];
	VkSwapchainKHR     swapchain;
	uint32_t           swapchain_size;
	VkExtent2D         swapchain_extent;
	VkSurfaceFormatKHR swapchain_format;
	VkImage            swapchain_images[5];
	VkImageView        swapchain_views[5];
	VkFence            queue_submit[MAX_FRAMES];
	VkSemaphore        acquire_image[MAX_FRAMES];
	VkSemaphore        present_image[MAX_FRAMES];
	uint32_t           current_frame;
	uint32_t           image_index;

	// --- graphics resources ---

	pipeline_t pipelines[PIPELINE_MAX];

};

extern context_t* g_context;

// --- helpers ---

void init_vulkan_core();
void init_pipelines();
void begin_frame();
void end_frame();

// --- internal ---

VkShaderModule create_shader_module(VkDevice device, const char* shader_path);
pipeline_t build_pipeline(const pipeline_info_t& info);
void recreate_swapchain();