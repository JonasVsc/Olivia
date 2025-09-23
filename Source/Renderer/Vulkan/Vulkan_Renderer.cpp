#include "Vulkan_Renderer.h"
#include <cassert>

VulkanRenderer* VulkanRenderer::Init(const RendererCreateInfo* rc)
{
	VulkanRenderer* r = new VulkanRenderer();

	if (r)
	{
		if (!r->create_device())
		{
			delete r;

			return nullptr;
		}
	}

	return r;
}

void VulkanRenderer::Destroy(VulkanRenderer* r)
{
	assert(r && "Invalid pointer");

	// Destroy Device

	delete r;
}

bool VulkanRenderer::create_device()
{
	return true;
}

bool VulkanRenderer::create_swapchain()
{
	return true;
}

bool VulkanRenderer::create_frames()
{
	return true;
}
