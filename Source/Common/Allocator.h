#pragma once
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cassert>

class Allocator
{
public:

	Allocator(size_t size);

	~Allocator();

	void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

	void Reset();

private:

	uint8_t* m_start = nullptr;
	size_t m_size = 0;
	size_t m_offset = 0;

};
