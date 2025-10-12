#pragma once
#include "Olivia_Platform.h"
#include "Olivia_Renderer.h"

namespace Olivia
{
	typedef uint32_t Entity;

	OLIVIA_DEFINE_HANDLE(World);

	struct TransformComponent
	{
		float position[3];
		float rotation[3];
		float scale[3];
	};

	typedef Mesh MeshComponent;

	World GetWorld();

	World CreateWorld(Allocator allocator);

	uint32_t GetEntityCount(World world);

	Entity CreateEntity();

	TransformComponent* AddTransformComponent(Entity entity, const TransformComponent* transform);

	MeshComponent* AddMeshComponent(Entity entity, Mesh mesh);

	TransformComponent* GetTransformComponent(Entity entity);

	MeshComponent* GetMeshComponent(Entity entity);

	// Helper
	void BuildTransformComponentToMatrix(const TransformComponent* transform, float* matrix);
}