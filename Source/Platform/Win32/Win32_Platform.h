#pragma once
#include <Windows.h>

#include "Platform/PlatformCreateInfo.h"

struct Allocator;

typedef void (PFN_Update)(void);

struct Module
{
	void* handle;
	PFN_Update* Update;
};

struct Win32Platform
{
public:

	static Win32Platform* Init(Allocator* allocator, const PlatformCreateInfo* pc);

	static void Destroy(Win32Platform* p);

	void Update();

	void PollEvents();

	bool IsRunning() const { return running; }

	void Quit() { running = false; }

	HWND GetHandle() const { return window; }

	static void* Allocate(size_t size);

	static void Free(void* mem);

private:

	bool create_window(const char* title, int32_t w, int32_t h);

	bool load_game_module();
	
private:

	HWND window;

	bool running;

	Module game_module;
};