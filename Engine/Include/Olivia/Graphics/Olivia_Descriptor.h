#pragma once
#include "Olivia_VulkanCore.h"

namespace Olivia
{
	enum DescriptorType
	{
		DESCRIPTOR_VIEW_PROJ,
		DESCRIPTOR_MAX
	};

	struct UniformData
	{
		glm::mat4 view;
		glm::mat4 projection;
	};

	class DescriptorManager
	{
	public:

		DescriptorManager(VulkanCore& core);
		DescriptorManager(const DescriptorManager&) = delete;
		~DescriptorManager();

		inline VkDescriptorSetLayout get_set_layout() const { return m_descriptor_set_layout; }
		inline VkDescriptorSet       get_descriptor(uint32_t frame) const { return m_descriptor_set[frame]; }
		inline Buffer                get_uniform_buffer(uint32_t frame) const { return m_uniform_buffer[frame]; }

	private:

		void init_uniforms();
		void create_descriptor_pool();
		void create_descriptor_set_layout();
		void allocate_descriptor_sets();

		VulkanCore&           m_core;
		VkDescriptorPool      m_descriptor_pool;
		VkDescriptorSetLayout m_descriptor_set_layout;
		VkDescriptorSet       m_descriptor_set[MAX_CONCURRENT_FRAMES];
		Buffer                m_uniform_buffer[MAX_CONCURRENT_FRAMES];
	};

} // Olivia