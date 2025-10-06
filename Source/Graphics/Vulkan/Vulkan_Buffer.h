#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

struct Buffer
{
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo info;
};

Buffer CreateBuffer(VmaAllocator allocator, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocationFlags, VkDeviceSize size);