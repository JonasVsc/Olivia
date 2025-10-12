#include "Olivia/Olivia_MeshPrimitives.h"

namespace Olivia
{
	Mesh CreateQuadMesh()
	{
		Renderer r = GetRenderer();

		Vertex vertices[]
		{
			{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
			{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
			{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
			{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }
		};

		uint32_t indices[]
		{
			0, 1, 2,
			2, 3, 0
		};

		size_t vertex_size = ARRAY_SIZE(vertices) * sizeof(vertices[0]);
		size_t index_size = ARRAY_SIZE(indices) * sizeof(indices[0]);


		MeshCreateInfo mesh_info
		{
			.vertex_data = vertices,
			.vertex_size = vertex_size,
			.index_data = indices,
			.index_size = index_size,
			.index_count = ARRAY_SIZE(indices)
		};

		return CreateMesh(r, &mesh_info);
	}
}