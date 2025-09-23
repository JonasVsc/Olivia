#pragma once
#include <vulkan/vulkan.h>

#include "Renderer/RendererCreateInfo.h"

class VulkanRenderer
{
public:

	static VulkanRenderer* Init(const RendererCreateInfo* rc);

	static void Destroy(VulkanRenderer* r);

private:

	bool create_device();

	bool create_swapchain();

	bool create_frames();

private:
	
};