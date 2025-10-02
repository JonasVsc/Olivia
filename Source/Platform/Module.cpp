#include "Module.h"
#include "Common/Allocator.h"

#include <Windows.h>

bool Module::Load(Allocator* allocator)
{
    if (handle)
    {
        FreeLibrary(static_cast<HMODULE>(handle));
    }

    while (!CopyFileA("OliviaGame.dll", "_OliviaModule.dll", 0)) Sleep(200);

    handle = LoadLibraryA("_OliviaModule.dll");
    if (!handle)
    {
        printf("Failed on loading _OliviaModule.dll\n");
        return false;
    }

    Func_Init = reinterpret_cast<PFN_Init>(GetProcAddress(static_cast<HMODULE>(handle), "Init"));
    Func_Update = reinterpret_cast<PFN_Update>(GetProcAddress(static_cast<HMODULE>(handle), "Update"));

    if (!memory)
    {
        PFN_GetStateSize GetStateSize = reinterpret_cast<PFN_GetStateSize>(GetProcAddress(static_cast<HMODULE>(handle), "GetStateSize"));
        size_t state_size = GetStateSize();

        memory = allocator->Allocate(state_size);
        Func_Init(memory);
    }

    return true;
}
