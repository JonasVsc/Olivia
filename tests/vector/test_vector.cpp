#include <catch2/catch_test_macros.hpp>
#include "olivia/core/vector.h"

TEST_CASE("Vector works")
{
	auto vec = olivia::create_vector<int32_t>(10);

	olivia::vector_push_back(vec, 1);
	olivia::vector_push_back(vec, 12);
	olivia::vector_push_back(vec, 3);
	olivia::vector_push_back(vec, 33);
	olivia::vector_push_back(vec, 9);

	printf("vector info:\n");
	printf("- size: %zu\n", vec.size);
	printf("- capacity: %zu\n", vec.capacity);

	printf("- data: [ ");
	for (size_t i = 0; i < vec.size; ++i)
	{
		printf("%d ", vec.data[i]);
	}
	printf("]\n");

	olivia::destroy_vector(vec);
}