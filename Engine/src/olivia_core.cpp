#include "olivia/core/olivia_core_internal.h"
#include "olivia/core/olivia_logger.h"

namespace olivia
{
	lifetime_allocator create_lifetime_allocator(size_t size)
	{
		lifetime_allocator allocator = static_cast<lifetime_allocator>(calloc(1, sizeof(lifetime_allocator_t)));

		if (allocator)
		{

			allocator->start = (uint8_t*)calloc(1, size);

			if (allocator->start)
			{
				allocator->size  = size;
				LOG_INFO(TAG_OLIVIA, "created life-time-allocator of size [%zu] bytes", allocator->size);
			}
			else
			{
				return nullptr;
			}
		}

		return allocator;
	}

	uint8_t* allocate(lifetime_allocator allocator, size_t size, size_t alignment)
	{
		uintptr_t current_addr = reinterpret_cast<uintptr_t>(allocator->start + allocator->offset);

		// Align up
		size_t aligned_addr = (current_addr + (alignment - 1)) & ~(alignment - 1);
		size_t padding      = aligned_addr - current_addr;

		if (allocator->offset + padding + size > allocator->size)
		{
			assert(false && "Allocator overflowed!");
			return nullptr;
		}

		allocator->offset += padding;
		uint8_t* result_ptr = allocator->start + allocator->offset;
		allocator->offset += size;

		LOG_INFO(TAG_OLIVIA, "life-time-allocator state: used [%zu] of [%zu] bytes", allocator->offset, allocator->size);

		return result_ptr;
	}

	void destroy_lifetime_allocator(lifetime_allocator allocator)
	{
		if (allocator)
		{
			if (allocator->start)
				free(allocator->start);

			free(allocator);
		}
	}


}