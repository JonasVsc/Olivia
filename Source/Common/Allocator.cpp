#include "Allocator.h"

#include "Defines.h"
#include "Platform/Platform.h"

void* BigAllocator::Allocate(size_t size, size_t alignment)
{
	size_t current = reinterpret_cast<size_t>(m_start + m_offset);

	// Align up
	size_t aligned = (current + (alignment - 1)) & ~(alignment - 1);
	size_t padding = aligned - current;

#ifdef OLIVIA_DEBUG
	printf("Allocator:\n"
		"- allocated: %zu (bytes)\n"
		"- in use: %zu (bytes)\n"
		"- max:  %zu (bytes)\n",
		size, m_offset + padding, m_size);
#endif // OLIVIA_DEBUG

	if (m_offset + padding + size > m_size)
	{
		assert(false && "Allocator overflowed!");
		return nullptr; // Out of memory
	}

	m_offset += padding;
	void* ptr = m_start + m_offset;
	m_offset += size;

	return ptr;
}

void BigAllocator::Reset()
{
	m_offset = 0;
}

BigAllocator::BigAllocator(size_t size)
{
	m_start = static_cast<uint8_t*>(Platform::Allocate(size));
	m_size = size;
	m_offset = 0;
}

BigAllocator::~BigAllocator()
{
	Platform::Free(m_start);
}

SmallAllocator::SmallAllocator(size_t size)
{
	m_start = new uint8_t[size];
	m_size = size;
	m_offset = 0;
}

SmallAllocator::~SmallAllocator()
{
	delete[] m_start;
}

void* SmallAllocator::Allocate(size_t size, size_t alignment)
{
	size_t current = reinterpret_cast<size_t>(m_start + m_offset);

	// Align up
	size_t aligned = (current + (alignment - 1)) & ~(alignment - 1);
	size_t padding = aligned - current;

#ifdef OLIVIA_DEBUG
	printf("Allocator:\n"
		"- allocated: %zu (bytes)\n"
		"- in use: %zu (bytes)\n"
		"- max:  %zu (bytes)\n",
		size, m_offset + padding, m_size);
#endif // OLIVIA_DEBUG

	if (m_offset + padding + size > m_size)
	{
		assert(false && "Allocator overflowed!");
		return nullptr; // Out of memory
	}

	m_offset += padding;
	void* ptr = m_start + m_offset;
	m_offset += size;

	return ptr;
}

void SmallAllocator::Reset()
{
	m_offset = 0;
}
