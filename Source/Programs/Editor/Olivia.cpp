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

    RendererCreateInfo renderer_create_info{
        
    };

    Renderer* renderer = Renderer::Init(&renderer_create_info);

    // Run
    {
        while (platform->IsRunning())
        {
            platform->PollEvents();
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