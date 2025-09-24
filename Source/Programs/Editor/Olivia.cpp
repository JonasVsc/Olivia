#include "Platform/Platform.h"
#include "Renderer/Renderer.h"

#ifdef OLIVIA_WIN32

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     PWSTR     pCmdLine,
    _In_     int       nShowCmd)
{
    WindowParams wp{ "Olivia Editor", 1280, 720 };

    PlatformCreateInfo platform_create_info{
        .pWindowParams = &wp
    };

    Platform* platform = Platform::Init(&platform_create_info);
    if (!platform)
    {
        abort();
        return -1;
    }

    RendererCreateInfo renderer_create_info{
        .pWindow = platform->GetHandle()
    };

    Renderer* renderer = Renderer::Init(&renderer_create_info);
    if (!renderer)
    {
        abort();
        return -1;
    }

    // Run
    {
        for (;;)
        {
            platform->PollEvents();

            if (!platform->IsRunning())
                break;

            renderer->BeginFrame();

            renderer->EndFrame();
        }
    }

    // Cleanup
    Renderer::Destroy(renderer);
    Platform::Destroy(platform);
    return EXIT_SUCCESS;
}

#else
    #error win32 only
#endif