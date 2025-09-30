#include "Vulkan_Graphics.h"
#include <cstdlib>

VulkanGraphics* VulkanGraphics::Init(VulkanRenderer* renderer)
{
	VulkanGraphics* g = static_cast<VulkanGraphics*>(calloc(1, sizeof(VulkanGraphics)));

	if (g)
	{

	}

	return g;
}

void VulkanGraphics::Destroy(VulkanGraphics* g)
{
	free(g);
}
