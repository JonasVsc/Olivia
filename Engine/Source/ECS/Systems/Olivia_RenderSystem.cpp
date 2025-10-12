#include "Olivia/Systems/Olivia_RenderSystem.h"
#include "Olivia/Olivia_Math.h"
#include <cstdio>

namespace Olivia
{
	constexpr uint32_t MAX_RENDERABLES{ 10'000 };

	enum PipelineType
	{
		PIPELINE_OPAQUE,
		PIPELINE_COUNT
	};

	struct RenderBatch
	{
		Entity entity[MAX_RENDERABLES];
		uint32_t entity_count;
	};

	struct RenderSystem_T
	{
		Renderer renderer;
		World world;

		GraphicsPipeline pipelines[PIPELINE_COUNT];

		Entity renderable_entities[MAX_RENDERABLES];
		size_t renderable_count;
	};

	RenderSystem CreateRenderSystem(Allocator allocator, Renderer renderer, World world)
	{
		RenderSystem render_system = (RenderSystem)Allocate(allocator, sizeof(RenderSystem_T));

		if (render_system)
		{
			render_system->world = world;
			render_system->renderer = renderer;


			GraphicsPipelineCreateInfo pipeline_info
			{
				.vertex_shader_path = "olivia_2d.vert.spv",
				.fragment_shader_path = "olivia_2d.frag.spv"
			};

			render_system->pipelines[PIPELINE_OPAQUE] = CreateGraphicsPipeline(renderer, &pipeline_info);
		}

		return render_system;
	}

	void RenderSystem_Render(RenderSystem render_system)
	{
		if (render_system->renderable_count == 0)
		{
			World world = render_system->world;
			uint32_t world_entity_count = GetEntityCount(world);

			printf("[RENDER_SYSTEM] Rendering World with [%u] entities!\n", world_entity_count);

			// Process entities ( Instance Creation )
			for (uint32_t world_entity = 0; world_entity < world_entity_count; ++world_entity)
			{
				if (*GetMeshComponent(world_entity) == UINT32_MAX || GetTransformComponent(world_entity) == nullptr)
				{
					continue;
				}

				InstanceCreateInfo instance_info{};
				instance_info.mesh = *GetMeshComponent(world_entity);
				BuildTransformComponentToMatrix(GetTransformComponent(world_entity), instance_info.transform);
				CreateInstance(render_system->renderer, &instance_info);

				render_system->renderable_count++;
			}

			PrepareRender(render_system->renderer);
		}
		else
		{
			Bind(render_system->renderer, render_system->pipelines[PIPELINE_OPAQUE]);
			Draw(render_system->renderer);
		}
	}
}