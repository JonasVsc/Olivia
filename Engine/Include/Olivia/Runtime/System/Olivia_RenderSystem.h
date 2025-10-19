#pragma once
#include "Olivia/Olivia_Runtime.h"
#include "Olivia/Olivia_Graphics.h"

namespace Olivia
{
	class RenderSystem
	{
	public:

		RenderSystem(Registry& registry, Renderer& renderer);
		RenderSystem(const RenderSystem&) = delete;

		void draw();

	private:

		Registry&             m_registry;
		Renderer&             m_renderer;
		VkPipeline            m_pipeline{};
		VkPipelineLayout      m_pipeline_layout{};
		VkDescriptorSetLayout m_set_layout{};
		VkDescriptorSet       m_descriptor_set[MAX_CONCURRENT_FRAMES]{};
		Buffer                m_uniform_buffer[MAX_CONCURRENT_FRAMES];
	};
}