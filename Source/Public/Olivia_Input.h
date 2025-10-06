#pragma once
#include <cstdint>

namespace Olivia
{
	struct InputAPI
	{
		bool (*IsKeyDown)(int32_t key);
		bool (*IsKeyPressed)(int32_t key);
	};
}