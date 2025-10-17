#pragma once
#include "Olivia_VulkanCore.h"
#include "Olivia_Pipeline.h"
#include "Olivia_Mesh.h"

#define VK_CHECK(x)														\
	do																	\
	{																	\
		VkResult err = x;												\
		if (err)														\
		{																\
			printf("Detected Vulkan error: %d", err);					\
			abort();													\
		}																\
	} while (0)

namespace Olivia
{
	class Renderer
	{
	public:

		Renderer(SDL_Window& window);
		Renderer(const Renderer&) = delete;
		~Renderer();

		void begin_frame();
		void end_frame();

		inline VkCommandBuffer get_cmd() { return m_core.get_frame_cmd(); }

		inline PipelineHandle get_pipeline(PipelineType type)
		{
			return m_pipeline.get_pipeline(type);
		}

	private:

		SDL_Window& m_window;
		VulkanCore  m_core{ m_window };
		Pipeline    m_pipeline{ m_core };
		Mesh        m_mesh{ m_core };
	};
}