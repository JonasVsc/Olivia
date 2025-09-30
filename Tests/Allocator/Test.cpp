#include <catch2/catch_test_macros.hpp>
#include "Common/Allocator.h"
#include "Common/Defines.h"

TEST_CASE("BigAllocator works")
{
	BigAllocator allocator(GIGABYTES(4));
	void* dummy[10];
	
	dummy[0]  = allocator.Allocate(MEGABYTES(200));
	dummy[1]  = allocator.Allocate(MEGABYTES(150));
	dummy[2]  = allocator.Allocate(MEGABYTES(300));
	dummy[3]  = allocator.Allocate(MEGABYTES(300));
	dummy[4]  = allocator.Allocate(MEGABYTES(125));
}

TEST_CASE("SmallAllocator works")
{
	SmallAllocator allocator(MEGABYTES(20));
	void* dummy[10];

	dummy[0] = allocator.Allocate(MEGABYTES(4));
	dummy[2] = allocator.Allocate(MEGABYTES(5));
	dummy[3] = allocator.Allocate(MEGABYTES(5));
	dummy[3] = allocator.Allocate(MEGABYTES(5));
}