#pragma once

#ifdef OLIVIA_VULKAN
#include "Vulkan/Vulkan_Graphics.h"
using Graphics = VulkanGraphics;
#else
#error vulkan only
#endif

