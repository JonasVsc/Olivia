#pragma once
#include "olivia_defines.h"

namespace olivia
{
	OLIVIA_DEFINE_HANDLE(lifetime_allocator);

	lifetime_allocator create_lifetime_allocator(size_t size);

	uint8_t* allocate(lifetime_allocator allocator, size_t size, size_t alignment = alignof(std::max_align_t));

	void destroy_lifetime_allocator(lifetime_allocator allocator);
}