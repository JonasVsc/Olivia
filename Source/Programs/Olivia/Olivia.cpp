#include "Public/Olivia.h"

#ifdef OLIVIA_WIN32

#include "Platform/Win32/Platform.h"
#include "Graphics/Vulkan/Vulkan_Renderer.h"

using namespace Olivia;

static bool g_running{ true };

API CreateOliviaAPI()
{
    API api{};
    api.input.IsKeyDown = Olivia::IsKeyDown;
    api.input.IsKeyPressed = Olivia::IsKeyPressed;
    return api;
}

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     PWSTR     pCmdLine,
    _In_     int       nShowCmd)
{
    EnableConsole();

    Allocator allocator = CreateAllocator(GIGABYTES(2));
    Window window = CreateWindowR("Teste", 640u, 480u);
    Renderer* renderer = CreateRenderer(&allocator, &window);

    // Load API
    API api = CreateOliviaAPI();

    // Load game DLL
    DLL game = LoadDLL(&allocator, &api);

    for (;;)
    {
        float dt = UpdateFrame();

        UpdateInput();

        if (not g_running) break;

        // Hot Reload
        if (IsKeyPressed(0x74)) // F5
        {
            ReloadDLL(&api, &game);
        }

        // Update game logic in DLL
        game.Update(dt);

        // Get render data from DLL state and render it
        Draw(renderer);
    }

    DestroyRenderer(renderer);
    DestroyWindowR(&window);
    FreeAllocator(&allocator);
    return EXIT_SUCCESS;
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
        PostQuitMessage(0);
        g_running = false;
        return 0;
    case WM_KEYDOWN:
        if (wParam >= 0 && wParam < 256)
        {
            GetInput()->curr_keys[wParam] = true;
        }
        break;
    case WM_KEYUP:
        if (wParam >= 0 && wParam < 256)
        {
            GetInput()->curr_keys[wParam] = false;
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

#else
    #error win32 only
#endif