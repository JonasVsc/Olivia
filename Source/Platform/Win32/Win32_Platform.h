#pragma once
#include <Windows.h>

#include "Platform/PlatformCreateInfo.h"
#include "Platform/Module.h"
#include "Shared/Input.h"

struct Allocator;

struct Win32Platform
{
public:

	static Win32Platform* Init(Allocator* allocator, const PlatformCreateInfo* pc);

	static void Destroy(Win32Platform* p);


	void Update();

	void PollEvents();

	bool IsRunning() const { return running; }

	HWND GetHandle() const { return window; }

	static void* Allocate(size_t size);

	static void Free(void* mem);

private:

	bool create_window(const char* title, int32_t w, int32_t h);
	
private:

	Allocator* allocator;

	HWND window;

	bool running;
	
	InputState input;
	
	Module main_module;

	LARGE_INTEGER frequency;
	LARGE_INTEGER last_frame;
	LARGE_INTEGER delta_itme;
};