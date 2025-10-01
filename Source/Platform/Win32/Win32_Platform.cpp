#include "Win32_Platform.h"
#include "Common/Allocator.h"

#include <cassert>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

Win32Platform* Win32Platform::Init(Allocator* allocator, const PlatformCreateInfo* pc)
{
#ifdef OLIVIA_DEBUG
    AllocConsole(); FILE* file;
    freopen_s(&file, "CONOUT$", "w", stdout);
#endif // OLIVIA_DEBUG

    Win32Platform* p = static_cast<Win32Platform*>(allocator->Allocate(sizeof(Win32Platform)));

    if (p)
    {
        if (!p->create_window(pc->pWindowParams->pTitle, pc->pWindowParams->width, pc->pWindowParams->height))
        {
            return nullptr;
        }

        if (!p->load_game_module())
        {
            return nullptr;
        }

        p->running = true;
    }

	return p;
}

void Win32Platform::Destroy(Win32Platform* p)
{
	assert(p && "Invalid pointer");
    DestroyWindow(p->window);
}

void Win32Platform::Update()
{
    if (game_module.Update)
    {
        game_module.Update();
    }
}

void Win32Platform::PollEvents()
{
    MSG msg = {};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            Quit();
            break;
        }

        if (msg.message == WM_KEYDOWN)
        {
            if (msg.wParam == VK_F5)
            {
                printf("Reloading modules\n");
                load_game_module();
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void* Win32Platform::Allocate(size_t size)
{
    void* mem = VirtualAlloc(
        nullptr,
        size,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);

    return mem;
}

void Win32Platform::Free(void* mem)
{
    VirtualFree(mem, 0, MEM_RELEASE);
}

bool Win32Platform::create_window(const char* title, int32_t w, int32_t h)
{
    HINSTANCE instance = (HINSTANCE)GetModuleHandle(NULL);
    const char CLASS_NAME[] = "Window Class";
    WNDCLASS windowClass = { 0 };
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = CLASS_NAME;
    RegisterClass(&windowClass);

    window = CreateWindowA(
        CLASS_NAME,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, w, h,
        nullptr, nullptr, instance, nullptr);

    if (!window)
    {
        return false;
    }

    ShowWindow(window, SW_SHOWNORMAL);

    return true;
}

bool Win32Platform::load_game_module()
{
    if (game_module.handle)
    {
        FreeLibrary(static_cast<HMODULE>(game_module.handle));
    }

    while (!CopyFileA("OliviaEditor.dll", "_OliviaEditor.dll", 0)) Sleep(200);

    game_module.handle = LoadLibraryA("_OliviaEditor.dll");
    if (!game_module.handle)
    {
        printf("Failed on loading _OliviaEditor.dll\n");
        return false;
    }

    game_module.Update = reinterpret_cast<PFN_Update*>(GetProcAddress(static_cast<HMODULE>(game_module.handle), "Update"));

    return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}