#pragma once
#include "olivia/olivia_renderer.h"

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
	};

	struct renderer_t
	{
		SDL_Window* window;

		vulkan_core_t core;
	};

	void init_vulkan_core(renderer renderer);

	void recreate_swapchain(renderer renderer);

} // olivia