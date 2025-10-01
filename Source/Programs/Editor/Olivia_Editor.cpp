#include <cstdio>

// This file is a test, will be removed later

#define OLIVIA_API extern "C" __declspec(dllexport)

OLIVIA_API void Update()
{
	printf("Updating game!\n");
}

