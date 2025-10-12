#pragma once
#include "Olivia_Core.h"

namespace Olivia
{
	uint32_t Clamp(uint32_t value, uint32_t lowest_value, uint32_t highest_value);

	void Mat4_Identity(float* mat4);
}