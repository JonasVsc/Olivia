#pragma once
#include "Olivia_Defines.h"

namespace Olivia
{
	// Platform

	void Init(size_t memory_size, const char* title, int32_t width, int32_t height);
	void Load(const char* program);
	void Update();
	bool Running();
	void Destroy();

	// Input

	bool IsKeyDown(int32_t key);
	bool IsKeyPressed(int32_t key);

} // Olivia