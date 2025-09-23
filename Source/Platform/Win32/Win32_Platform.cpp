#include "Win32_Platform.h"
#include <cassert>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

Win32Platform* Win32Platform::Init(const PlatformCreateInfo* pc)
{
	Win32Platform* p = new Win32Platform();

    if (p)
    {
        if (!p->create_window(pc->pWindowParams->pTitle, pc->pWindowParams->width, pc->pWindowParams->height))
        {
            delete p;
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

	delete p;
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

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
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
        PostQuitMessage(0);    // posts WM_QUIT
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}