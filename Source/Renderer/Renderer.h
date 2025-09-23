#pragma once

#ifdef OLIVIA_VULKAN
	#include "Vulkan/Vulkan_Renderer.h"
	using Renderer = VulkanRenderer;
#else
	#error vulkan only
#endif

