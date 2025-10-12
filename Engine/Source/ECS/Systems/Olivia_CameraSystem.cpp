#include "Olivia/Systems/Olivia_CameraSystem.h"
#include "Olivia/Olivia_Math.h"

namespace Olivia
{
	struct CameraSystem_T
	{
		Renderer renderer;
		World world;

		ViewProjection view_proj;
	};
	CameraSystem CreateCameraSystem(Allocator allocator, Renderer renderer, World world)
	{
		CameraSystem camera_system = (CameraSystem)Allocate(allocator, sizeof(CameraSystem_T));

		if (camera_system)
		{
			camera_system->renderer = renderer;
			camera_system->world = world;

			Mat4_Identity(camera_system->view_proj.projection);
			Mat4_Identity(camera_system->view_proj.view);
		}

		return camera_system;
	}
	void CameraSystem_Update(CameraSystem camera_system)
	{
		SetViewProjection(camera_system->renderer, &camera_system->view_proj);
	}
}