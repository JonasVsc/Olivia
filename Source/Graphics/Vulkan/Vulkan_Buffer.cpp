#include "Vulkan_Buffer.h"
#include "Common/Allocator.h"

Buffer CreateBuffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size)
{
	Buffer b{};

	VkBufferCreateInfo buffer_create_info{};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = (VkDeviceSize)size;
	buffer_create_info.usage = usage_flags;

	VmaAllocationCreateInfo allocation_create_info{};
	allocation_create_info.usage = memory_usage;
	allocation_create_info.flags = allocation_flags;

	assert(vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &b.buffer, &b.allocation, &b.info) == VK_SUCCESS);

	return b;
}
