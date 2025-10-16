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
}