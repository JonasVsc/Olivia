#pragma once
#include "Olivia/Olivia_ECS.h"
#include "Olivia/Olivia_Renderer.h"

namespace Olivia
{
	OLIVIA_DEFINE_HANDLE(RenderSystem);

	RenderSystem CreateRenderSystem(Allocator allocator, Renderer renderer, World world);

	void RenderSystem_Render(RenderSystem render_system);

} // Olivia