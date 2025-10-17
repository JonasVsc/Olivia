#pragma once
#include "Olivia_Graphics.h"

namespace Olivia
{
	enum MeshType
	{
		MESH_QUAD,
		MESH_MAX
	};

	class Mesh
	{
		struct MeshInfo
		{
			uint32_t index_count;
			uint32_t index_offset;
			uint32_t vertex_count;
			uint32_t vertex_offset;
		};

	public:

		Mesh(VulkanCore& core);
		Mesh(const Mesh&) = delete;
		~Mesh();

		void upload_mesh(const MeshInfo& info, MeshType type);


	private:

		VulkanCore& m_core;
		Buffer      m_vertex_buffer{};
		Buffer      m_index_buffer{};
		MeshInfo    m_mesh[MESH_MAX]{};
	};

} // Olivia