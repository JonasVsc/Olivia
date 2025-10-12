#pragma once

#define OLIVIA_GAME extern "C" __declspec(dllexport)

#define OLIVIA_DEFINE_HANDLE(object) typedef struct object##_T* object;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define KILOBYTES(x) ((x) * 1024ull)
#define MEGABYTES(x) (KILOBYTES(x) * 1024ull)
#define GIGABYTES(x) (MEGABYTES(x) * 1024ull)

#include <cstdint>
#include <cstddef>
