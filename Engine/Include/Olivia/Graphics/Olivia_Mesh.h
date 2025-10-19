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

		inline VkBuffer& get_vertex_buffer() { return m_vertex_buffer.buffer; }
		inline VkBuffer& get_index_buffer() { return m_index_buffer.buffer; }
		inline MeshInfo& get_mesh(uint32_t id) { return m_mesh[id]; }

	private:

		VulkanCore& m_core;
		Buffer      m_vertex_buffer{};
		Buffer      m_index_buffer{};
		MeshInfo    m_mesh[MESH_MAX]{};
	};

} // Olivia