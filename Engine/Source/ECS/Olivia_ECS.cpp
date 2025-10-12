#include "Olivia/Olivia_ECS.h"
#include <cstring>

namespace Olivia
{
	constexpr uint32_t MAX_ENTITIES{ 10'000 };

	struct World_T
	{
		TransformComponent transform[MAX_ENTITIES];
		MeshComponent mesh[MAX_ENTITIES];
		uint32_t entity_count;
	};

	static World g_world{ nullptr };

	World GetWorld()
	{
		return g_world;
	}

	World CreateWorld(Allocator allocator)
	{
		World world = (World)Allocate(allocator, sizeof(World_T));

		if (world)
		{
			g_world = world;
			memset(world->mesh, UINT32_MAX, sizeof(world->mesh));
		}

		return world;
	}

	uint32_t GetEntityCount(World world)
	{
		return world->entity_count;
	}


	Entity CreateEntity()
	{
		World cs = GetWorld();
		return cs->entity_count++;
	}

	TransformComponent* AddTransformComponent(Entity entity, const TransformComponent* transform)
	{
		World cs = GetWorld();
		TransformComponent* entity_transform = &cs->transform[entity];
		
		memcpy(entity_transform, transform, sizeof(TransformComponent));

		return entity_transform;
	}

	MeshComponent* AddMeshComponent(Entity entity, Mesh mesh)
	{
		World cs = GetWorld();
		MeshComponent* entity_mesh = &cs->mesh[entity];

		*entity_mesh = mesh;

		return entity_mesh;
	}

	TransformComponent* GetTransformComponent(Entity entity)
	{
		World cs = GetWorld();
		TransformComponent* entity_transform = &cs->transform[entity];

		return entity_transform;
	}


	MeshComponent* GetMeshComponent(Entity entity)
	{
		World cs = GetWorld();
		return &cs->mesh[entity];
	}

	void BuildTransformComponentToMatrix(const TransformComponent* transform, float* matrix)
	{
		// ROW 0
		matrix[0] = transform->scale[0];
		matrix[1] = 0.0f;
		matrix[2] = 0.0f;
		matrix[3] = transform->position[0];

		// ROW 1
		matrix[4] = 0.0f;
		matrix[5] = transform->scale[1];
		matrix[6] = 0.0f;
		matrix[7] = transform->position[1];

		// ROW 2
		matrix[8]  = 0.0f;
		matrix[9]  = 0.0f;
		matrix[10] = transform->scale[2];
		matrix[11] = transform->position[2];

		// ROW 3
		matrix[12] = 0.0f;
		matrix[13] = 0.0f;
		matrix[14] = 0.0f;
		matrix[15] = 1.0f;

	}


}