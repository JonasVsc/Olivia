#pragma once
#include <Windows.h>

#include "Platform/PlatformCreateInfo.h"

struct Win32Platform
{
public:

	static Win32Platform* Init(const PlatformCreateInfo* pc);

	static void Destroy(Win32Platform* p);

	void PollEvents();

	bool IsRunning() const { return running; }

	void Quit() { running = false; }

	HWND GetHandle() const { return window; }

private:

	bool create_window(const char* title, int32_t w, int32_t h);

private:

	HWND window;

	bool running;
};