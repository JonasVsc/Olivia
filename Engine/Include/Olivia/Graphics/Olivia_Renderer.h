#pragma once
#include "Olivia_VulkanCore.h"
#include "Olivia_Descriptor.h"
#include "Olivia_Pipeline.h"
#include "Olivia_Mesh.h"
#include "Olivia_Instance.h"

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

		inline VmaAllocator    get_allocator() { return m_core.get_allocator(); }
		inline uint32_t        get_frame() { return m_core.m_current_frame; }
		inline VkCommandBuffer get_cmd() { return m_core.get_frame_cmd(); }

	private:

		SDL_Window& m_window;
		VulkanCore  m_core{ m_window };

	public:

		DescriptorManager  m_descriptor_manager{ m_core };
		PipelineManager    m_pipeline_manager{ m_core };
		MeshAtlas          m_mesh_atlas{ m_core };
		InstanceCache      m_instance_cache{ m_core };
	};
}