#pragma once
#include <cstdint>

struct WindowParams
{
	const char* pTitle;
	int32_t width, height;
};

struct PlatformCreateInfo
{
	const WindowParams* window_params;
};