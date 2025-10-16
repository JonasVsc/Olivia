#include "Olivia/Olivia.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     PWSTR     pCmdLine,
    _In_     int       nShowCmd)
{

    Olivia::Init(MEGABYTES(3), "Olivia Engine", 640, 480);

    Olivia::Load("Olivia_Program.dll");

    while (Olivia::Running())
    {
        Olivia::Update();

        Olivia::BeginFrame();

        Olivia::EndFrame();
    }
    
    Olivia::Destroy();
    return 0;
}