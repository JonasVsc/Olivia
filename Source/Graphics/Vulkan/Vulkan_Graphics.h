#pragma once
#include "Vulkan_Buffer.h"

class VulkanRenderer;

class VulkanGraphics
{
public:
	
	static VulkanGraphics* Init(VulkanRenderer* renderer);

	static void Destroy(VulkanGraphics* graphics);

private:


private:

	VulkanBuffer* buffers;
	uint32_t buffer_count;
};