#pragma once
#include "Olivia/Olivia_Core.h"
#include "Olivia_Component.h"

namespace Olivia
{
	using EntityID = uint32_t;

	constexpr uint32_t MAX_ENTITIES{ 2'000 };

	struct EntityCreateInfo
	{
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
		uint32_t  mesh;
		bool      is_renderable;
		bool      is_player;
	};
	class Renderer;
	class Registry
	{
	public:

		Registry(Renderer& renderer);
		Registry(const Registry&) = delete;
		~Registry();

		EntityID create(const EntityCreateInfo& info);
		
		void update_transform(EntityID id, const glm::mat4& transform);
		
		inline EntityID    get_player() { return m_player; }
		inline CTransform& get_transform(EntityID id) { return m_transform_pool[id]; }


	private:

		Renderer&  m_renderer;

		CMesh      m_mesh_pool[MAX_ENTITIES]{};
		CTransform m_transform_pool[MAX_ENTITIES]{};
		EntityID   m_created_entities{};
		EntityID   m_player;
	};

} // Olivia