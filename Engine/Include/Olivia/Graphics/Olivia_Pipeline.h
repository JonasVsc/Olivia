#pragma once
#include "Olivia_VulkanCore.h"

namespace Olivia
{
	using PipelineID = uint32_t;

	constexpr uint32_t MAX_PIPELINES{ 10 };

	struct PipelineInfo
	{
		const char*            vertex_shader_path{};
		const char*            fragment_shader_path{};
		uint32_t               set_layout_count{};
		VkDescriptorSetLayout* set_layouts{};
	};

	class PipelineManager
	{
	public:

		PipelineManager(VulkanCore& core);
		PipelineManager(const PipelineManager&) = delete;
		~PipelineManager();

		PipelineID create_pipeline(const PipelineInfo& info);

		inline VkPipeline       get_pipeline(PipelineID id) { return m_pipeline[id]; }
		inline VkPipelineLayout get_pipeline_layout(PipelineID id) { return m_layout[id]; }

	private:

		VkShaderModule create_shader_module(VkDevice device, const char* shader_path);

		VulkanCore&      m_core;
		VkPipeline	     m_pipeline[MAX_PIPELINES]{};
		VkPipelineLayout m_layout[MAX_PIPELINES]{};
		PipelineID       m_created_pipelines{};
	};

} // Olivia