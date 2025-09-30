#pragma once
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cassert>

class BigAllocator
{
public:

	BigAllocator(size_t size);

	~BigAllocator();

	void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

	void Reset();

private:

	uint8_t* m_start = nullptr;
	size_t m_size = 0;
	size_t m_offset = 0;


};

class SmallAllocator
{
public:

	SmallAllocator(size_t size);

	~SmallAllocator();

	void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

	void Reset();

private:

	uint8_t* m_start = nullptr;
	size_t m_size = 0;
	size_t m_offset = 0;

};

