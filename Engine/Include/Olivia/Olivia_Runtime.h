#pragma once
#include "Olivia_Core.h"
#include "Olivia_ECS.h"
#include "Olivia_MeshPrimitives.h"

namespace Olivia
{
	struct API
	{
		float (*DeltaTime)(void);
		
		bool (*IsKeyDown)(int32_t);
		bool (*IsKeyPressed)(int32_t);

		Mesh(*CreateQuadMesh)(void);

		Entity (*CreateEntity)(void);
		TransformComponent* (*AddTransformComponent)(Entity, const TransformComponent*);
		TransformComponent* (*GetTransformComponent)(Entity);
		MeshComponent* (*AddMeshComponent)(Entity, Mesh);
		MeshComponent* (*GetMeshComponent)(Entity);

	};
}