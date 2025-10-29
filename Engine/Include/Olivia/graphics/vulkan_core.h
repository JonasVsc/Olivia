#pragma once
#include "olivia/olivia_core.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

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

namespace olivia
{
	constexpr uint32_t MAX_FRAMES{ 2 };

	struct vulkan_core_t
	{
		SDL_Window*        window;
		VkInstance         instance;
		VkSurfaceKHR       surface;
		VkPhysicalDevice   gpu;
		VkQueue            queue;
		uint32_t           graphics_queue_index;
		VkDevice           device;
		VmaAllocator       allocator;
		VkCommandPool      command_pool;
		VkCommandBuffer    command_buffers[5];
		VkSwapchainKHR     swapchain;
		uint32_t           swapchain_size;
		VkExtent2D         swapchain_extent;
		VkSurfaceFormatKHR swapchain_format;
		VkImage            swapchain_images[5];
		VkImageView        swapchain_views[5];
		VkSemaphore        present_image[5];
		VkSemaphore        acquire_image[MAX_FRAMES];
		VkFence            queue_submit[MAX_FRAMES];
		uint32_t           current_frame;
		uint32_t           image_index;
	};

	void init_vulkan_core(SDL_Window* window);

	void destroy_vulkan_core();

	void recreate_swapchain();

	void begin_frame();

	void end_frame();
}

