#pragma once
#include "Olivia_Platform.h"
#include "Olivia_Renderer_Impl.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Olivia
{
	typedef void (*Program_Init)(void*);
	typedef void (*Program_Update)(void);

	struct InputState
	{
		uint8_t current_keys[256];
		uint8_t previous_keys[256];
	};

	struct PlatformStorage
	{
		uint8_t* start;
		size_t offset;
		size_t size;
	};

	struct Platform
	{
		bool running;
		
		//WINDOW

		void* window;

		// INPUT

		InputState input;
		
		// PROGRAM

		void* program;
		const char* program_name;
		Program_Update Update;

		// RENDERER

		Renderer renderer;
	};

	extern Platform* g_platform;

	// HELPERS

	void* PlatformStorageAllocate(size_t size);
	void* ReadFileFromPath(const char* file_path, size_t* file_size);
	void  FreeFile(void* file);
}