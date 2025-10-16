#pragma once

#define OLIVIA_API extern "C" __declspec(dllexport)

#define OLIVIA_DEFINE_HANDLE(object) typedef struct object##_Impl* object

#ifdef OLIVIA_DEBUG
	#define TAG_OLIVIA   "\033[97m[OLIVIA]\033[0m"
	#define TAG_PLATFORM "\033[97m[PLATFORM]\033[0m"
	#define TAG_RENDERER "\033[97m[RENDERER]\033[0m"
	#define TAG_PROGRAM  "\033[97m[PROGRAM]\033[0m"

	#define LOG_INFO(tag, fmt, ...)   printf(tag " \033[97m" fmt "\033[0m\n", ##__VA_ARGS__)
	#define LOG_WARN(tag, fmt, ...)   printf(tag " \033[93m" fmt "\033[0m\n", ##__VA_ARGS__)
	#define LOG_ERROR(tag, fmt, ...)  printf(tag " \033[91m" fmt "\033[0m\n", ##__VA_ARGS__)
#else 
	#define LOG_INFO(tag, fmt, ...)  
	#define LOG_WARN(tag, fmt, ...)  
	#define LOG_ERROR(tag, fmt, ...) 
#endif

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define KILOBYTES(x) ((x) * 1024ull)
#define MEGABYTES(x) (KILOBYTES(x) * 1024ull)
#define GIGABYTES(x) (MEGABYTES(x) * 1024ull)

#include <stdexcept>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <cstdio>