#include <Olivia/Olivia.h>

using namespace Olivia;


API BuildOliviaAPI()
{
    API olivia
    {
        .DeltaTime = DeltaTime,

        .IsKeyDown = IsKeyDown,
        .IsKeyPressed = IsKeyPressed,

        .CreateQuadMesh = CreateQuadMesh,

        .CreateEntity = CreateEntity,

        .AddTransformComponent = AddTransformComponent,
        .GetTransformComponent = GetTransformComponent,

        .AddMeshComponent = AddMeshComponent,
        .GetMeshComponent = GetMeshComponent
    };

    return olivia;
}

#ifdef OLIVIA_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     PWSTR     pCmdLine,
    _In_     int       nShowCmd)
{
    EnableConsole();

    Allocator allocator = CreateAllocator(MEGABYTES(3));
    Window window = CreateWindowR(allocator, "Olivia Engine", 640, 480);
    Renderer renderer = CreateRenderer(allocator, window);
    World world = CreateWorld(allocator);
    API olivia = BuildOliviaAPI();
    
    GraphicsPipelineCreateInfo opaque_pipeline_info
    {
        .vertex_shader_path = "olivia_2d.vert.spv",
        .fragment_shader_path = "olivia_2d.frag.spv"
    };

    GraphicsPipeline opaque_pipeline = CreateGraphicsPipeline(renderer, &opaque_pipeline_info);

    RenderSystem render_system = CreateRenderSystem(allocator, renderer, world);

    DLL game_dll = LoadDLL(allocator, &olivia);

    for (;;)
    {
        UpdateFrame();

        PollEvents();

        if (!IsRunning()) break;

        if (IsKeyPressed(0x74)) // F5
        {
            ReloadDLL(game_dll);
        }

        Update(game_dll);

        BeginFrame(renderer);

        RenderSystem_Render(render_system);

        EndFrame(renderer);
    }

    DestroyRenderer(renderer);
    DestroyWindowR(window);
    DestroyAllocator(allocator);

    return 0;
}
#endif // OLIVIA_WIN32