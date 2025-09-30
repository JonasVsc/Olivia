#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "Renderer/RendererCreateInfo.h"

constexpr uint32_t MAX_CONCURRENT_FRAMES{ 2 };

struct Allocator;

class VulkanRenderer
{
public:

	static VulkanRenderer* Init(Allocator* allocator, const RendererCreateInfo* rc);

	static void Destroy(VulkanRenderer* r);

	void BeginFrame();

	void EndFrame();

private:

	bool create_device();

	bool create_swapchain();

	bool create_frames();

private:

	void* hWindow{ nullptr };

	bool vsync{ false };

	VkInstance instance{ VK_NULL_HANDLE };

	VkSurfaceKHR surface{ VK_NULL_HANDLE };

	VkPhysicalDevice gpu{ VK_NULL_HANDLE };

	VkQueue queue{ VK_NULL_HANDLE };

	uint32_t graphics_queue_index{ 0 };

	VkDevice device{ VK_NULL_HANDLE };

	VmaAllocator allocator{ VK_NULL_HANDLE };

	VkCommandPool command_pool{ VK_NULL_HANDLE };

	VkCommandBuffer command_buffers[5];

	VkSwapchainKHR swapchain{ VK_NULL_HANDLE };

	uint32_t swapchain_size{ 0 };

	VkExtent2D swapchain_extent{};

	VkImage swapchain_images[5];

	VkImageView swapchain_views[5];

	VkSurfaceFormatKHR swapchain_format;

	VkFence queue_submit[MAX_CONCURRENT_FRAMES];

	VkSemaphore acquire_image[MAX_CONCURRENT_FRAMES];

	VkSemaphore present_image[5];

	uint32_t current_frame{ 0 };

	uint32_t image_index{ 0 };

};