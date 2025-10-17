#pragma once
#include "Olivia_VulkanCore.h"

namespace Olivia
{
	enum MeshType
	{
		MESH_QUAD,
		MESH_MAX
	};

	struct MeshInfo
	{
		uint32_t index_count;
		uint32_t index_offset;
		uint32_t vertex_count;
		uint32_t vertex_offset;
	};

	class MeshAtlas
	{
	public:

		MeshAtlas(VulkanCore& core);
		MeshAtlas(const MeshAtlas&) = delete;
		~MeshAtlas();

		void upload_mesh(const MeshInfo& info, MeshType type);

		Buffer      m_vertex_buffer{};
		Buffer      m_index_buffer{};
		MeshInfo    m_mesh[MESH_MAX]{};

	private:

		VulkanCore& m_core;
	};

} // Olivia