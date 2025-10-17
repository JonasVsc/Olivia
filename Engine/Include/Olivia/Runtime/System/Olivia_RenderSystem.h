#pragma once
#include "Olivia/Olivia_Graphics.h"


namespace Olivia
{
	class RenderSystem
	{
	public:

		RenderSystem(Renderer& renderer);
		RenderSystem(const RenderSystem&) = delete;

		void draw();

	private:

		Renderer&        m_renderer;
		VkDescriptorPool m_descriptor_pool;
		VkPipeline       m_pipeline;
		VkPipelineLayout m_layout;
	};
}