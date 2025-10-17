#pragma once
#include "Olivia_Graphics.h"

namespace Olivia
{
	enum PipelineType
	{
		PIPELINE_OPAQUE,
		PIPELINE_MAX
	};

	struct PipelineHandle
	{
		VkPipeline       pipeline;
		VkPipelineLayout layout;
	};

	class Pipeline
	{
		struct PipelineInfo
		{
			const char* vertex_shader_path{};
			const char* fragment_shader_path{};
		};

	public:

		Pipeline(VulkanCore& core);
		Pipeline(const Pipeline&) = delete;
		~Pipeline();

		inline PipelineHandle get_pipeline(PipelineType type)
		{
			return PipelineHandle{ m_pipeline[type], m_layout[type] };
		}

	private:

		void           create_pipeline(const PipelineInfo& info, PipelineType type);
		VkShaderModule create_shader_module(VkDevice device, const char* shader_path);

		VulkanCore&      m_core;
		VkPipeline	     m_pipeline[PIPELINE_MAX];
		VkPipelineLayout m_layout[PIPELINE_MAX];
	};

} // Olivia