#include "Common/Allocator.h"

#include "Platform/Platform.h"
#include "Shared/Input.h"

#include <cstdio>

// This file is a sample, will be moved soon when gui project creator/selector is ready

#define OLIVIA_API extern "C" __declspec(dllexport)

struct GameState
{
	const char* player;
	bool alive;
};

// Return the State size
OLIVIA_API size_t GetStateSize()
{
	return sizeof(GameState);
}

// Initialize the Game GameState
OLIVIA_API void Init(void* state_memory)
{
	// Init GameState
	GameState* state = static_cast<GameState*>(state_memory);
	state->player = "JonasVsc";
}

// Run per frame
OLIVIA_API void Update(void* state_memory, InputState* input, float dt)
{
	// Game Logic
	GameState* state = static_cast<GameState*>(state_memory);

	if (input->IsKeyDown('W'))
	{
		printf("W");
	}
	if (input->IsKeyDown('A'))
	{
		printf("A");
	}
	if (input->IsKeyDown('S'))
	{
		printf("S");
	}
	if (input->IsKeyDown('D'))
	{
		printf("D");
	}
}

