#include "Olivia/Olivia_Math.h"

namespace Olivia
{
	uint32_t Clamp(uint32_t value, uint32_t lowest_value, uint32_t highest_value)
	{
		if (value > lowest_value && value < highest_value)
		{
			return value;
		}
		else if (value < lowest_value)
		{
			return lowest_value;
		}

		return highest_value;
	}

	void Mat4_Identity(float* mat4)
	{
		mat4[0] = 1.0f;
		mat4[5] = 1.0f;
		mat4[10] = 1.0f;
		mat4[15] = 1.0f;
	}
}