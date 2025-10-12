#include "Vulkan_Helpers.h"
#include "Platform/Win32/Platform.h"

/// @brief Helper macro to test the result of Vulkan calls which can return an error.
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
	Buffer CreateBuffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size)
	{
		VkBufferCreateInfo buffer_create_info
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = (VkDeviceSize)size,
			.usage = usage_flags
		};

		VmaAllocationCreateInfo allocation_create_info
		{
			.flags = allocation_flags,
			.usage = memory_usage
		};

		Buffer b{};
		VK_CHECK(vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &b.buffer, &b.allocation, &b.info));

		return b;
	}

	VkShaderModule CreateShaderModule(VkDevice device, const char* shader)
	{
		size_t code_size;
		uint8_t* shader_code = static_cast<uint8_t*>(ReadFileR(shader, &code_size));

		VkShaderModuleCreateInfo shader_module_ci
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = code_size,
			.pCode = (const uint32_t*)shader_code
		};

		VkShaderModule shader_module{ VK_NULL_HANDLE };
		VK_CHECK(vkCreateShaderModule(device, &shader_module_ci, nullptr, &shader_module));

		return shader_module;
	}
} // Olivia