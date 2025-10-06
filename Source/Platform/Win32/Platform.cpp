#include "Platform.h"
#include "Public/Olivia.h"

#include <cassert>
#include <cstring>


LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Olivia
{

	static Input g_input{};

	static LARGE_INTEGER g_frequency{};
	static LARGE_INTEGER g_prev_time{};


	Input* GetInput()
	{
		return &g_input;
	}

	void UpdateInput()
	{
		Input* input = GetInput();

		memcpy(input->prev_keys, input->curr_keys, sizeof(input->curr_keys));

		MSG msg{};
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	DLL LoadDLL(Allocator* allocator, API* api)
	{
		DLL dll{};

		uint32_t copy_tries{ 0 };
		while (!CopyFileA("Game.dll", "_Game.dll", 0))
		{
			Sleep(200);
			if (copy_tries > 10)
			{
				printf("Failed to load dll");
				abort();
			}

			copy_tries++;
		}

		dll.handle = LoadLibraryA("_Game.dll");
		if (not dll.handle)
		{
			printf("Failed to load dll");
			abort();
		}

		dll.Update = reinterpret_cast<DLL_Update>(GetProcAddress(static_cast<HMODULE>(dll.handle), "Update"));
		assert(dll.Update && "Failed to get DLL Update()");

		// Load permanent memory
		DLL_Init Init = reinterpret_cast<DLL_Init>(GetProcAddress(static_cast<HMODULE>(dll.handle), "Init"));
		DLL_GetStateSize GetStateSize = reinterpret_cast<DLL_GetStateSize>(GetProcAddress(static_cast<HMODULE>(dll.handle), "GetStateSize"));

		assert(Init && "Failed to get DLL Init()");
		assert(GetStateSize && "Failed to get DLL GetStateSize()");

		size_t state_size = GetStateSize();
		dll.memory = Allocate(allocator, state_size);

		Init(api, dll.memory);

		return dll;
	}

	void ReloadDLL(API* api, DLL* dll)
	{
		if (dll->handle)
		{
			FreeLibrary(static_cast<HMODULE>(dll->handle));
		}

		uint32_t copy_tries{ 0 };
		while (!CopyFileA("Game.dll", "_Game.dll", 0))
		{
			Sleep(200);
			if (copy_tries > 10)
			{
				printf("Failed to load dll");
				abort();
			}

			copy_tries++;
		}

		dll->handle = LoadLibraryA("_Game.dll");
		if (!dll->handle)
		{
			printf("Failed to load dll");
			abort();
		}

		dll->Update = reinterpret_cast<DLL_Update>(GetProcAddress(static_cast<HMODULE>(dll->handle), "Update"));
		assert(dll->Update && "Failed to get DLL Update()");

		DLL_Init Init = reinterpret_cast<DLL_Init>(GetProcAddress(static_cast<HMODULE>(dll->handle), "Init"));
		assert(Init && "Failed to get DLL Init()");

		Init(api, dll->memory);
	}

	float UpdateFrame()
	{
		LARGE_INTEGER current_time;
		QueryPerformanceCounter(&current_time);

		if (g_prev_time.QuadPart == 0)
		{
			QueryPerformanceFrequency(&g_frequency);
			g_prev_time = current_time;
			return 0.016f;
		}

		float dt = (float)(current_time.QuadPart - g_prev_time.QuadPart) / g_frequency.QuadPart;
		g_prev_time = current_time;

		return dt;
	}

	Allocator CreateAllocator(size_t size)
	{
		Allocator allocator{};

		void* mem = VirtualAlloc(
			nullptr,
			size,
			MEM_RESERVE | MEM_COMMIT,
			PAGE_READWRITE);

		allocator.start = static_cast<uint8_t*>(mem);
		allocator.size = size;
		allocator.offset = 0;

		return allocator;
	}

	void* Allocate(Allocator* allocator, size_t size, size_t alignment)
	{
		size_t current = reinterpret_cast<size_t>(allocator->start + allocator->offset);

		// Align up
		size_t aligned = (current + (alignment - 1)) & ~(alignment - 1);
		size_t padding = aligned - current;

		if (allocator->offset + padding + size > allocator->size)
		{
			assert(false && "Allocator overflowed!");
			return nullptr; // Out of memory
		}

		allocator->offset += padding;
		void* ptr = allocator->start + allocator->offset;
		allocator->offset += size;

		return ptr;
	}

	void FreeAllocator(Allocator* allocator)
	{
		VirtualFree(allocator->start, 0, MEM_RELEASE);
	}

	void EnableConsole()
	{
		AllocConsole(); FILE* file;
		freopen_s(&file, "CONOUT$", "w", stdout);
	}

	Window CreateWindowR(const char* title, uint32_t w, uint32_t h)
	{
		Window window{};

		HINSTANCE instance = (HINSTANCE)GetModuleHandle(NULL);
		const char CLASS_NAME[] = "Window Class";
		WNDCLASS windowClass = { 0 };
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = instance;
		windowClass.lpszClassName = CLASS_NAME;
		RegisterClass(&windowClass);

		window.handle = CreateWindowA(
			CLASS_NAME,
			title,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, w, h,
			nullptr, nullptr, instance, nullptr);

		assert(window.handle && "Failed to create window");

		if (window.handle)
		{
			ShowWindow(static_cast<HWND>(window.handle), SW_SHOWNORMAL);
		}

		window.width = w;
		window.height = h;

		return window;
	}

	void DestroyWindowR(Window* window)
	{
		DestroyWindow(static_cast<HWND>(window->handle));
	}

}