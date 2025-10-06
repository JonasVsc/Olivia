// This file is a sample, will be moved soon when gui project creator/selector is ready
#include "Public/Olivia.h"
#include <cstdio>

#define OLIVIA_API extern "C" __declspec(dllexport)

using namespace Olivia;

struct GameState
{
	bool initialized;
	const char* player;
	int32_t alive;
};

static GameState* state = nullptr;
static API* api = nullptr;

// Return the State size
OLIVIA_API size_t GetStateSize()
{
	return sizeof(GameState);
}

// Initialize the Game GameState
OLIVIA_API void Init(API* _api, void* _state_memory)
{
	api = _api;
	state = static_cast<GameState*>(_state_memory);

	// Persisted
	if (!state->initialized)
	{
		state->initialized = true;
		state->alive = 1;
	}

	// Transient
	state->player = "JonasVsc";

	printf("[DLL] Initialized game state and api\n");
}

// Run per frame
OLIVIA_API void Update(float dt)
{
	// Game Logic
	if (api->input.IsKeyPressed('A'))
	{
		printf("%s\n", state->player);
		printf("%d\n", state->alive);

		state->alive++;
	}
}
