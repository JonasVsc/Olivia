#pragma once
#include "olivia_memory.h"

namespace olivia
{
	struct lifetime_allocator_t
	{
		uint8_t* start;
		size_t size;
		size_t offset;
	};
}