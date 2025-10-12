#pragma once
#include "Olivia/Olivia_Core.h"

namespace Olivia
{
	struct API;

	OLIVIA_DEFINE_HANDLE(Allocator);
	OLIVIA_DEFINE_HANDLE(Window);
	OLIVIA_DEFINE_HANDLE(DLL);

	bool IsKeyDown(int32_t key);

	bool IsKeyPressed(int32_t key);

	float DeltaTime();

	void EnableConsole();

	Allocator CreateAllocator(size_t size);

	void* Allocate(Allocator allocator, size_t size, size_t alignment = alignof(std::max_align_t));

	void DestroyAllocator(Allocator allocator);

	Window CreateWindowR(Allocator allocator, const char* title, uint32_t w, uint32_t h);

	void GetWindowSize(Window window, uint32_t* width, uint32_t* height);

	void DestroyWindowR(Window window);

	void PollEvents();

	bool IsRunning();

	DLL LoadDLL(Allocator allocator, API* api);

	void ReloadDLL(DLL dll);

	void Update(DLL dll);

	void UpdateFrame();

	void* ReadFileR(const char* file_path, size_t* file_size);

	void FreeFileR(void* handle);

	bool WindowCreateSurface(void* instance, Window window, void** surface);
}