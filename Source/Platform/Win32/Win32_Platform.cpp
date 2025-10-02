#include "Win32_Platform.h"
#include "Common/Allocator.h"

#include <cassert>
#include <cstring>

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
        p->allocator = allocator;

        if (!p->create_window(pc->pWindowParams->pTitle, pc->pWindowParams->width, pc->pWindowParams->height))
        {
            return nullptr;
        }

        if (!p->main_module.Load(allocator))
        {
            return nullptr;
        }

        QueryPerformanceFrequency(&p->frequency);
        QueryPerformanceCounter(&p->last_frame);

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
    // Calculate delta time
    LARGE_INTEGER  curr_frame;
    QueryPerformanceCounter(&curr_frame);
    float dt = static_cast<float>(curr_frame.QuadPart - last_frame.QuadPart) / frequency.QuadPart;
    last_frame = curr_frame;

    if (main_module.Func_Update)
    {
        main_module.Func_Update(main_module.memory, &input, dt);
    }
}

void Win32Platform::PollEvents()
{
    memcpy(input.previousKeys, input.currentKeys, sizeof(input.currentKeys));

    MSG msg = {};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
        case WM_QUIT:
            running = false;
            break;
        case WM_KEYDOWN:

            if (msg.wParam >= 0 && msg.wParam < 256)
            {
                input.currentKeys[msg.wParam] = true;
            }
            break;

        case WM_KEYUP:
            if (msg.wParam >= 0 && msg.wParam < 256)
            {
                input.currentKeys[msg.wParam] = false;
            }
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (input.IsKeyPressed(VK_F5))
    {
        printf("Reloading modules\n");
        main_module.Load(allocator);
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