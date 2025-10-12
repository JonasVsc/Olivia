#include <Olivia/Olivia.h>
#include <cstdio>

using namespace Olivia;

struct Player
{
	Entity id;
	MeshComponent* mesh;
	TransformComponent* transform;
};

struct Game
{
	bool initialized;
	Mesh mesh_quad;

	// Entities
	Player player;
};

Game* game;
API* olivia;

OLIVIA_GAME void Init(void* _memory, API* _api)
{
	game = (Game*)_memory;
	olivia = _api;

	if (!game->initialized)
	{
		game->initialized = true;

		game->mesh_quad = olivia->CreateQuadMesh();


		// CREATE ENTITY 0
		Entity id0 = olivia->CreateEntity();

		olivia->AddMeshComponent(id0, game->mesh_quad);

		TransformComponent transform
		{
			.position = {0.0f, 0.0f, 0.0f},
			.rotation = {0.0f, 0.0f, 0.0f},
			.scale = {10.0f, 14.0f, 11.0f}
		};

		olivia->AddTransformComponent(id0, &transform);

		// CREATE ENTITY 1
		Entity id1 = olivia->CreateEntity();

		olivia->AddMeshComponent(id1, game->mesh_quad);

		olivia->AddTransformComponent(id1, &transform);

		printf("[DLL] Initializing game\n");
	}

	printf("[DLL] Reloading dll\n");
}

OLIVIA_GAME void Update()
{
	if (olivia->IsKeyPressed('F'))
	{
		//printf("ENTITY %u\n", game->player.id);
		//printf("- mesh: %u\n", *game->player.mesh);
		//printf("- transform: %u\n", *game->player.mesh);
		//printf("  - position: (%.2f, %.2f, %.2f)\n", game->player.transform->position[0], game->player.transform->position[1], game->player.transform->position[2]);
		//printf("  - rotation: (%.2f, %.2f, %.2f)\n", game->player.transform->rotation[0], game->player.transform->rotation[1], game->player.transform->rotation[2]);
		//printf("  - scale: (%.2f, %.2f, %.2f)\n", game->player.transform->scale[0], game->player.transform->scale[1], game->player.transform->scale[2]);
	}
	if (olivia->IsKeyDown('D'))
	{
		printf("%f\n", olivia->DeltaTime());
	}

}
