#pragma once
#include "Olivia/Olivia_ECS.h"
#include "Olivia/Olivia_Renderer.h"

/*
	1. Querying all entities that have the necessary components for rendering(e.g., a Transform and a Mesh).
	2. Sorting Batching these entities for efficient rendering.
	3. Issuing Commands to the graphics API(like OpenGL, Vulkan, or DirectX) to draw them.
*/
namespace Olivia
{
	OLIVIA_DEFINE_HANDLE(CameraSystem);

	CameraSystem CreateCameraSystem(Allocator allocator, Renderer renderer, World world);

	void CameraSystem_Update(CameraSystem camera_system);

} // Olivia