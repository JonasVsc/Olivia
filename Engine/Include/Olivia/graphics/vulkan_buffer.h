#pragma once
#include "vulkan_core.h"

namespace olivia
{
	struct vulkan_buffer_t
	{
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo info;
	};

	vulkan_buffer_t create_vulkan_buffer(VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size);

	void destroy_vulkan_buffer(vulkan_buffer_t& buffer);
}