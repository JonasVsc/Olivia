#pragma once

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <cstdio>

#define OLIVIA_API extern "C" __declspec(dllexport)

#define OLIVIA_DEFINE_HANDLE(object) typedef struct object##_Impl* object

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define KILOBYTES(x) ((x) * 1024ull)
#define MEGABYTES(x) (KILOBYTES(x) * 1024ull)
#define GIGABYTES(x) (MEGABYTES(x) * 1024ull)

