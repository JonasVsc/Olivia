#pragma once
#include "Olivia_VulkanCore.h"

namespace Olivia
{
	constexpr uint32_t MAX_INSTANCES{ 20'000 };
	constexpr uint32_t MAX_BATCHES{ 200 };

	struct InstanceBatch 
	{
		uint32_t mesh;
		uint32_t instance_offset;
		uint32_t instance_count;
	};

	class InstanceCache
	{
	public:
		
		InstanceCache(VulkanCore& core);
		InstanceCache(const InstanceCache&) = delete;
		~InstanceCache();

		void update();
		void cache_it(const Instance& instance, uint32_t mesh_id);


		inline void           clear_cache() { m_instance_count = 0; }
		inline uint32_t       get_instance_count() { return m_instance_count; }
		inline Instance&      get_instance(uint32_t i) { return m_instances[i]; }
		inline uint32_t       get_batch_count() { return m_batch_count; }
		inline InstanceBatch& get_batch(uint32_t i) { return m_batches[i]; }

		void bind();

	private:

		VulkanCore& m_core;

		Buffer      m_instance_buffer{};
		Instance    m_instances[MAX_INSTANCES]{};
		uint32_t    m_instance_count{};


		InstanceBatch m_batches[MAX_BATCHES]{};
		uint32_t      m_batch_count{};
	};

} // Olivia