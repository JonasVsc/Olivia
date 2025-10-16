#pragma once
#include "Olivia/Olivia_Defines.h"

#define WIN32_LEAN_AND_MEAH
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_win32.h>
#include <glm/glm.hpp>


namespace Olivia
{
	// Renderer
	void DestroyRenderer();

	void BeginFrame();
	void EndFrame();

} // Olivia