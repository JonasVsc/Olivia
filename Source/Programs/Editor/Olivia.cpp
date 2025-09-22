#include "Platform/Platform.h"

#ifdef OLIVIA_WIN32

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     PWSTR     pCmdLine,
    _In_     int       nShowCmd)
{
    WindowParams wp{ "Olivia Editor", 1280, 720 };

    PlatformCreateInfo platform_create_info{
        .window_params = &wp
    };

    Platform* platform = Platform::Init(&platform_create_info);
    
    // Run
    {
        while (platform->IsRunning())
        {
            platform->PollEvents();
        }
    }

    // Cleanup
    Platform::Destroy(platform);
    return EXIT_SUCCESS;
}

#else
    #error win32 only
#endif