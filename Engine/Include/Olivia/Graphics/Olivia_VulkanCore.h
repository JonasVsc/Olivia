#pragma once
#include "Olivia/Olivia_Core.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <glm/glm.hpp>


namespace Olivia
{
	constexpr uint32_t MAX_CONCURRENT_FRAMES{ 2 };

	struct Buffer
	{
		VkBuffer          buffer;
		VmaAllocation     allocation;
		VmaAllocationInfo info;
	};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct Instance
	{
		glm::mat4 transform;
	};

	class VulkanCore
	{
	public:

		VulkanCore(SDL_Window& window);
		VulkanCore(const VulkanCore&) = delete;
		~VulkanCore();

		void   recreate_swapchain();

		inline VkDevice        get_device() { return m_device; }
		inline VmaAllocator    get_allocator() { return m_allocator; }
		inline VkCommandBuffer get_frame_cmd() { return m_command_buffers[m_current_frame]; }

		VkInstance         m_instance{};
		VkSurfaceKHR       m_surface{};
		VkPhysicalDevice   m_gpu{};
		VkQueue            m_queue{};
		uint32_t           m_graphics_queue_index{};
		VkDevice           m_device{};
		VmaAllocator       m_allocator{};
		VkCommandPool      m_command_pool{};
		VkCommandBuffer    m_command_buffers[5]{};
		VkSwapchainKHR     m_swapchain{};
		uint32_t           m_swapchain_size{};
		VkExtent2D         m_swapchain_extent{};
		VkImage            m_swapchain_images[5]{};
		VkImageView        m_swapchain_views[5]{};
		VkSurfaceFormatKHR m_swapchain_format{};
		VkFence            m_queue_submit[MAX_CONCURRENT_FRAMES]{};
		VkSemaphore        m_acquire_image[MAX_CONCURRENT_FRAMES]{};
		VkSemaphore        m_present_image[5]{};
		uint32_t           m_current_frame{};
		uint32_t           m_image_index{};
		VkFence            m_imm_fence{};
		VkCommandBuffer    m_imm_command_buffer{};

	private:

		SDL_Window& m_window;

		void create_device();
		void create_swapchain();
		void create_sync_objects();
	};

	Buffer create_buffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size);

} // Olivia