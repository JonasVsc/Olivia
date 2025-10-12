#pragma once
#include "Olivia_Core.h"
#include "Olivia_Platform.h"

namespace Olivia
{
	typedef uint32_t Mesh;
	typedef uint32_t Instance;

	OLIVIA_DEFINE_HANDLE(Renderer);
	OLIVIA_DEFINE_HANDLE(GraphicsPipeline);

	struct ViewProjection
	{
		float projection[16];
		float view[16];
	};

	struct Vertex
	{
		float position[3];
		float normal[3];
		float color[4];
		float uv[2];
	};

	struct GraphicsPipelineCreateInfo
	{
		const char* vertex_shader_path;
		const char* fragment_shader_path;
		// ...
	};

	struct MeshCreateInfo
	{
		const void* vertex_data;
		size_t vertex_size;

		const void* index_data;
		size_t index_size;
		uint32_t index_count;
	};

	struct InstanceCreateInfo
	{
		float transform[16]; // model matrix;
		Mesh mesh;
	};

	Renderer CreateRenderer(Allocator allocator, Window window);

	Renderer GetRenderer();

	void DestroyRenderer(Renderer renderer);

	void BeginFrame(Renderer renderer);

	void Draw(Renderer renderer);

	void EndFrame(Renderer renderer);

	GraphicsPipeline CreateGraphicsPipeline(Renderer renderer, const GraphicsPipelineCreateInfo* info);

	void Bind(Renderer renderer, GraphicsPipeline pipeline);

	Mesh CreateMesh(Renderer renderer, const MeshCreateInfo* info);

	Instance CreateInstance(Renderer renderer, const InstanceCreateInfo* info);

	void PrepareRender(Renderer renderer);

	void SetViewProjection(Renderer renderer, const ViewProjection* view_projection);

} // Olivia