#include "Olivia/Olivia_Runtime.h"
#include "Olivia/Olivia_Graphics.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Olivia
{
	Registry::Registry(Renderer& renderer)
		: m_renderer{ renderer }
	{

	}

	Registry::~Registry()
	{

	}

	EntityID Registry::create(const EntityCreateInfo& info)
	{
		EntityID id = m_created_entities;

		// MESH COMPONENT
		m_mesh_pool[id] = m_renderer.m_mesh_atlas.get_mesh(info.mesh);

		// TRANSFORM COMPONENT
		m_transform_pool[id].position = info.position;
		m_transform_pool[id].rotation = info.rotation;
		m_transform_pool[id].scale    = info.scale;

		if (info.is_player)
		{
			m_player = id;
		}

		if (info.is_renderable)
		{
			glm::mat4 transform = glm::mat4(1.0f);
			transform = glm::translate(transform, info.position);
			transform = glm::scale(transform, info.scale);

			m_renderer.m_instance_cache.cache_it
			({
				.transform = transform,
				
			}, info.mesh);
		}

		return m_created_entities++;
	}

	void Registry::update_transform(EntityID id, const glm::mat4& transform)
	{
		m_renderer.m_instance_cache.get_instance(id).transform = transform;
	}

	RenderSystem::RenderSystem(Registry& registry, Renderer& renderer)
		: m_registry{ registry }
		, m_renderer{ renderer }
	{
		// GET UNIFORM BUFFERS
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{

			m_uniform_buffer[i] = m_renderer.m_descriptor_manager.get_uniform_buffer(i);
			m_descriptor_set[i] = m_renderer.m_descriptor_manager.get_descriptor(i);
		}

		VkDescriptorSetLayout set_layout = m_renderer.m_descriptor_manager.get_set_layout();

		// CREATE PIPELINE
		PipelineID id = m_renderer.m_pipeline_manager.create_pipeline
		({
			.vertex_shader_path   = "olivia.vert.spv",
			.fragment_shader_path = "olivia.frag.spv",
			.set_layout_count     = 1,
			.set_layouts          = &set_layout
		});

		m_pipeline        = m_renderer.m_pipeline_manager.get_pipeline(id);
		m_pipeline_layout = m_renderer.m_pipeline_manager.get_pipeline_layout(id);
	}

	void RenderSystem::draw()
	{
		// update instance data
		m_renderer.m_instance_cache.update();

		VkCommandBuffer cmd = m_renderer.get_cmd();

		VkDeviceSize offsets[]{ 0 };
		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 
			0, 1, &m_descriptor_set[m_renderer.get_frame()],
			0, nullptr);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		vkCmdBindVertexBuffers(cmd, 0, 1, &m_renderer.m_mesh_atlas.get_vertex_buffer(), offsets);
		vkCmdBindIndexBuffer(cmd, m_renderer.m_mesh_atlas.get_index_buffer(), 0, VK_INDEX_TYPE_UINT32);
		m_renderer.m_instance_cache.bind();

		for (uint32_t i = 0; i < m_renderer.m_instance_cache.get_batch_count(); ++i)
		{
			const InstanceBatch& batch = m_renderer.m_instance_cache.get_batch(i);
			const MeshInfo& mesh_info = m_renderer.m_mesh_atlas.get_mesh(batch.mesh);

			vkCmdDrawIndexed(cmd, mesh_info.index_count, batch.instance_count, 0, 0, batch.instance_offset);
		}
	}

	MovementSystem::MovementSystem(Registry& registry)
		: m_registry{ registry }
	{
	}

	void MovementSystem::update()
	{
		float dt = 0.016f;
		float speed = 10.0f * dt;
		CTransform& transform = m_registry.get_transform(m_registry.get_player());

		if (is_key_down(SDL_SCANCODE_W)) transform.position.y += speed;
		if (is_key_down(SDL_SCANCODE_S)) transform.position.y -= speed;
		if (is_key_down(SDL_SCANCODE_A)) transform.position.x -= speed;
		if (is_key_down(SDL_SCANCODE_D)) transform.position.x += speed;

		// Rebuild transform matrix
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, transform.position);
		model = glm::scale(model, transform.scale);


		LOG_INFO(TAG_OLIVIA, "Payer %d -> (%.1f, %.1f)", m_registry.get_player(), transform.position.x, transform.position.y);
	}


} // Olivia