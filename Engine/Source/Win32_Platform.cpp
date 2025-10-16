#include "Olivia/Olivia_Platform_Impl.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Olivia
{
	static PlatformStorage g_platform_storage{};
	Platform* g_platform{ nullptr };

	void Init(size_t memory_size, const char* title, int32_t width, int32_t height)
	{

		// CONSOLE

		AllocConsole(); FILE* file;
		freopen_s(&file, "CONOUT$", "w", stdout);

		HANDLE cmd = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD mode = 0;
		GetConsoleMode(cmd, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(cmd, mode);

		// ALLOC

		g_platform_storage.start = (uint8_t*)VirtualAlloc(
			nullptr,
			memory_size,
			MEM_RESERVE | MEM_COMMIT,
			PAGE_READWRITE);

		g_platform_storage.size = memory_size;

		g_platform = (Platform*)PlatformStorageAllocate(sizeof(Platform));

		// WINDOW

		HINSTANCE instance = (HINSTANCE)GetModuleHandle(NULL);
		const char CLASS_NAME[] = "Window Class";
		WNDCLASS windowClass = { 0 };
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = instance;
		windowClass.lpszClassName = CLASS_NAME;
		RegisterClass(&windowClass);

		g_platform->window = CreateWindowA(
			CLASS_NAME,
			title,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, width, height,
			nullptr, nullptr, instance, nullptr);

		ShowWindow((HWND)g_platform->window, SW_SHOWNORMAL);

		// RENDERER

		CreateDevice();

		CreateSwapchain();

		CreateSyncObjects();

		CreateMeshAtlasBuffer();

		CreatePipelines();

		// PROGRAM

		g_platform->program = (g_platform_storage.start + g_platform_storage.offset);
	}

	void Load(const char* program_name)
	{
		uint32_t copy_tries{ 0 };
		while (!CopyFileA(program_name, "_Olivia_Program.dll", 0))
		{
			Sleep(200);
			if (copy_tries > 10)
			{
				abort();
			}
			copy_tries++;
		}

		g_platform->program = LoadLibraryA("_Olivia_Program.dll");
		g_platform->program_name = program_name;

		if (g_platform->program)
		{
			g_platform->Update = (Program_Update)GetProcAddress((HMODULE)g_platform->program, "Olivia_Update");
			assert(g_platform->Update && "Failed to get program Update()");

			Program_Init Init = (Program_Init)GetProcAddress((HMODULE)g_platform->program, "Olivia_Init");
			assert(Init && "Failed to get DLL Init()");

			Init(g_platform_storage.start + g_platform_storage.offset);
			g_platform->running = true;
		}
	}

	void Update()
	{
		memcpy(g_platform->input.previous_keys, g_platform->input.current_keys, sizeof(g_platform->input.current_keys));

		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (IsKeyPressed(0x74)) // F5
		{
			if (g_platform->program)
			{
				FreeLibrary((HMODULE)g_platform->program);
			}

			uint32_t copy_tries{ 0 };
			while (!CopyFileA(g_platform->program_name, "_Olivia_Program.dll", 0))
			{
				Sleep(200);
				if (copy_tries > 10)
				{
					abort();
				}
				copy_tries++;
			}

			g_platform->program = LoadLibraryA("_Olivia_Program.dll");

			if (g_platform->program)
			{
				g_platform->Update = (Program_Update)GetProcAddress((HMODULE)g_platform->program, "Olivia_Update");
				assert(g_platform->Update && "Failed to get program Update()");

				Program_Init Init = (Program_Init)GetProcAddress((HMODULE)g_platform->program, "Olivia_Init");
				assert(Init && "Failed to get DLL Init()");

				LOG_INFO(TAG_PROGRAM, "Reloading");
				Init(g_platform_storage.start + g_platform_storage.offset);
			}
		}

		g_platform->Update();
	}

	bool Running()
	{
		return g_platform->running;
	}

	void Destroy()
	{
		if (g_platform->program)
		{
			DestroyRenderer();
			DestroyWindow((HWND)g_platform->window);
			FreeLibrary((HMODULE)g_platform->program);
		}
	}

	bool IsKeyDown(int32_t k)
	{
		return (k >= 0 && k < 256) ? g_platform->input.current_keys[k] : false;
	}

	bool IsKeyPressed(int32_t k)
	{
		return (k >= 0 && k < 256) ? (g_platform->input.current_keys[k] && !g_platform->input.previous_keys[k]) : false;
	}

	void* PlatformStorageAllocate(size_t size)
	{
		void* ptr = (Platform*)(g_platform_storage.start + g_platform_storage.offset);
		g_platform_storage.offset += size;
		return ptr;
	}

	void* ReadFileFromPath(const char* file_path, size_t* file_size)
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
					return nullptr;
				}

				*file_size = size;
				return buffer;
			}
		}

		return nullptr;
	}

	void  FreeFile(void* file)
	{
		HeapFree(GetProcessHeap(), 0, file);
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
		Olivia::g_platform->running = false;
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		if (wParam >= 0 && wParam < 256)
		{
			Olivia::g_platform->input.current_keys[wParam] = true;
		}
		break;
	case WM_KEYUP:
		if (wParam >= 0 && wParam < 256)
		{
			Olivia::g_platform->input.current_keys[wParam] = false;
		}
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}