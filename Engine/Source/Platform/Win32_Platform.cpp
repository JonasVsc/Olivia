#include "Olivia/Olivia_Platform.h"
#include "Olivia/Olivia_Runtime.h"

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef OLIVIA_VULKAN
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#endif

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Olivia
{
	typedef void(*DLL_Init)(void*, API*);
	typedef void(*DLL_Update)(void);

	struct InputState
	{
		bool curr_keys[256];
		bool prev_keys[256];
	};

	struct Allocator_T
	{
		uint8_t* start;
		size_t size;
		size_t offset;
	};

	struct Window_T
	{
		HWND handle;
		uint32_t width, height;
		const char* title;
		bool close;
	};

	struct DLL_T
	{
		void* handle;
		void* memory;
		API* api;
		const char* file;
		DLL_Update Update;
	};

	static bool g_running{};
	static InputState g_input_state{};
	static LARGE_INTEGER g_frequency{};
	static LARGE_INTEGER g_prev_time{};
	static float g_delta_time{};

	bool IsKeyDown(int32_t k)
	{
		return (k >= 0 && k < 256) ? g_input_state.curr_keys[k] : false;
	}

	bool IsKeyPressed(int32_t k)
	{
		return (k >= 0 && k < 256) ? (g_input_state.curr_keys[k] && !g_input_state.prev_keys[k]) : false;
	}

	float DeltaTime()
	{
		return g_delta_time;
	}

	bool IsRunning()
	{
		return g_running;
	}

	void StopRunning()
	{
		g_running = false;
	}

	InputState* GetInputState()
	{
		return &g_input_state;
	}

	LARGE_INTEGER GetFrequency()
	{
		return g_frequency;
	}

	LARGE_INTEGER GetPreviousTime()
	{
		return g_prev_time;
	}

	void EnableConsole()
	{
		AllocConsole(); FILE* file;
		freopen_s(&file, "CONOUT$", "w", stdout);
	}

	Allocator CreateAllocator(size_t size)
	{
		Allocator allocator = (Allocator)calloc(1, sizeof(Allocator_T));

		if (allocator)
		{
			void* mem = VirtualAlloc(
				nullptr,
				size,
				MEM_RESERVE | MEM_COMMIT,
				PAGE_READWRITE);

			allocator->start = static_cast<uint8_t*>(mem);
			allocator->size = size;
			allocator->offset = 0;
		}

		return allocator;
	}
	
	void* Allocate(Allocator allocator, size_t size, size_t alignment)
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

		printf("[ALLOCATOR] Used: [%zu] of [%zu]\n", allocator->offset, allocator->size);

		return ptr;
	}

	void DestroyAllocator(Allocator allocator)
	{
		VirtualFree(allocator->start, 0, MEM_RELEASE);
	}

	Window CreateWindowR(Allocator allocator, const char* title, uint32_t w, uint32_t h)
	{
		Window window = (Window)Allocate(allocator, sizeof(Window_T));

		HINSTANCE instance = (HINSTANCE)GetModuleHandle(NULL);
		const char CLASS_NAME[] = "Window Class";
		WNDCLASS windowClass = { 0 };
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = instance;
		windowClass.lpszClassName = CLASS_NAME;
		RegisterClass(&windowClass);

		window->handle = CreateWindowA(
			CLASS_NAME,
			title,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, w, h,
			nullptr, nullptr, instance, nullptr);

		assert(window->handle && "Failed to create window");

		if (window->handle)
		{
			ShowWindow(static_cast<HWND>(window->handle), SW_SHOWNORMAL);
		}

		window->width = w;
		window->height = h;
		g_running = true;

		return window;
	}

	void GetWindowSize(Window window, uint32_t* width, uint32_t* height)
	{
		*width  = window->width;
		*height = window->height;
	}

	bool WindowShouldClose(Window window)
	{
		return window->close;
	}

	void DestroyWindowR(Window window)
	{
		DestroyWindow(window->handle);
	}

	void PollEvents()
	{
		InputState* input = GetInputState();

		memcpy(input->prev_keys, input->curr_keys, sizeof(input->curr_keys));

		MSG msg{};
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	DLL LoadDLL(Allocator allocator, API* api)
	{
		DLL dll = (DLL)Allocate(allocator, sizeof(DLL_T));

		uint32_t copy_tries{ 0 };
		while (!CopyFileA("Game.dll", "_Game.dll", 0))
		{
			Sleep(200);
			if (copy_tries > 10)
			{
				return nullptr;
			}
			
			copy_tries++;
		}

		dll->handle = LoadLibraryA("_Game.dll");

		if (dll->handle)
		{
			dll->Update = (DLL_Update)GetProcAddress((HMODULE)dll->handle, "Update");
			assert(dll->Update && "Failed to get DLL Update()");

			DLL_Init Init = (DLL_Init)GetProcAddress((HMODULE)dll->handle, "Init");
			assert(Init && "Failed to get DLL Init()");

			dll->memory = Allocate(allocator, MEGABYTES(1));
			dll->api = api;

			Init(dll->memory, dll->api);
		}

		return dll;
	}

	void ReloadDLL(DLL dll)
	{
		if (dll->handle)
		{
			FreeLibrary((HMODULE)dll->handle);
		}

		uint32_t copy_tries{ 0 };
		while (!CopyFileA("Game.dll", "_Game.dll", 0))
		{
			Sleep(200);
			if (copy_tries > 10)
			{
				abort();
			}

			copy_tries++;
		}

		dll->handle = LoadLibraryA("_Game.dll");

		if (dll->handle)
		{
			dll->Update = (DLL_Update)GetProcAddress((HMODULE)dll->handle, "Update");
			assert(dll->Update && "Failed to get DLL Update()");

			DLL_Init Init = (DLL_Init)GetProcAddress((HMODULE)dll->handle, "Init");
			assert(Init && "Failed to get DLL Init()");

			Init(dll->memory, dll->api);
		}
	}

	void Update(DLL dll)
	{
		dll->Update();
	}

	void UpdateFrame()
	{
		LARGE_INTEGER current_time;
		QueryPerformanceCounter(&current_time);

		if (g_prev_time.QuadPart == 0)
		{
			QueryPerformanceFrequency(&g_frequency);
			g_prev_time = current_time;
			g_delta_time = 0.016f;
			return;
		}

		g_delta_time = (float)(current_time.QuadPart - g_prev_time.QuadPart) / g_frequency.QuadPart;
		g_prev_time = current_time;
	}

	void* ReadFileR(const char* file_path, size_t* file_size)
	{
		HANDLE heap = GetProcessHeap();

		HANDLE file = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file)
		{
			DWORD size = GetFileSize(file, NULL);

			if (size == INVALID_FILE_SIZE)
			{
				CloseHandle(file);
				abort();
			}

			void* buffer = HeapAlloc(heap, HEAP_ZERO_MEMORY, size);

			if (buffer)
			{
				DWORD bytesRead = 0;
				BOOL res = ReadFile(file, buffer, size, &bytesRead, NULL);
				CloseHandle(file);

				if (!res || bytesRead != size)
				{
					HeapFree(heap, 0, buffer);
					return NULL;
				}

				*file_size = size;
				return buffer;
			}
		}

		return nullptr;
	}

	void FreeFileR(void* handle)
	{
		HeapFree(GetProcessHeap(), 0, handle);
	}

	bool WindowCreateSurface(void* instance, Window window, void** surface)
	{
		VkWin32SurfaceCreateInfoKHR win32_surface_create_info
		{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = static_cast<HWND>(window->handle),
		};

		if (vkCreateWin32SurfaceKHR((VkInstance)instance, &win32_surface_create_info, nullptr, (VkSurfaceKHR*)surface) != VK_SUCCESS)
		{
			return false;
		}

		return true;
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SYSCOMMAND:
		// Block minimize and maximize commands
		switch (wParam & 0xFFF0)
		{
		case SC_MINIMIZE:
		case SC_MAXIMIZE:
		case SC_SIZE:
			return 0;
		}
		break;
	case WM_DESTROY:
		Olivia::StopRunning();
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		if (wParam >= 0 && wParam < 256)
		{
			Olivia::GetInputState()->curr_keys[wParam] = true;
		}
		break;
	case WM_KEYUP:
		if (wParam >= 0 && wParam < 256)
		{
			Olivia::GetInputState()->curr_keys[wParam] = false;
		}
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}