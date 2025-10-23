#include "olivia/runtime/olivia_runtime_internal.h"

namespace olivia
{
	static registry local_current_registry{ nullptr };


	registry create_registry()
	{
		registry result_registry = (registry)calloc(1, sizeof(registry_t));

		if (result_registry)
		{
			LOG_INFO(TAG_OLIVIA, "allocated registry of [%zu] bytes", sizeof(registry_t));

			local_current_registry = result_registry;
		}

		return result_registry;
	}

	void bind_registry(registry registry)
	{
		LOG_INFO(TAG_OLIVIA, "registry: binded registry -> total_entities: %u, total_instances: %u, batches: %u", registry->entity_count, registry->total_instance_count, registry->batch_count);

		for (uint32_t i = 0; i < registry->batch_count; ++i)
		{
			LOG_INFO(TAG_OLIVIA, "batch: %u:\n"
				"- instance_mesh: %u\n"
				"- instance_count: %u\n"
				"- instance_offset: %u\n",
				i, registry->instance_mesh[i], registry->instance_count[i], registry->instance_offset[i]);

		}
		local_current_registry = registry;
	}

	entity_t create_entity(registry registry, const entity_info_t& info)
	{
		entity_t current_entity = registry->entity_count;

		registry->cmesh_pool[current_entity]       = info.mesh;
		registry->cposition_pool[current_entity].x = info.position.x;
		registry->cposition_pool[current_entity].y = info.position.y;
		registry->cposition_pool[current_entity].z = info.position.z;
		registry->crotation_pool[current_entity].x = info.rotation.x;
		registry->crotation_pool[current_entity].y = info.rotation.y;
		registry->crotation_pool[current_entity].z = info.rotation.z;
		registry->cscale_pool[current_entity].x    = info.scale.x;
		registry->cscale_pool[current_entity].y    = info.scale.y;
		registry->cscale_pool[current_entity].z    = info.scale.z;

		if (registry->cscale_pool[current_entity].x > 0.0f)
		{
			uint32_t current_batch = request_instance_batch(registry, info.mesh);

			if (!registry->batch_initialized[current_batch])
			{
				registry->batch_initialized[current_batch] = true;
				registry->instance_mesh[current_batch]     = info.mesh;
				registry->instance_offset[current_batch]   = 0;
				registry->instance_count[current_batch]++;

				registry->previous_instance_offset = registry->instance_count[current_batch] * sizeof(instance_t);
			}
			else
			{
				registry->instance_offset[current_batch] = registry->previous_instance_offset;
			}

			registry->instance_mesh[current_batch] = info.mesh;
			registry->instance_count[current_batch]++;

			registry->previous_instance_offset = registry->instance_count[current_batch] * sizeof(instance_t);

			// upload instance to buffer

			const float c3 = cos(info.rotation.z);
			const float s3 = sin(info.rotation.z);
			const float c2 = cos(info.rotation.x);
			const float s2 = sin(info.rotation.x);
			const float c1 = cos(info.rotation.y);
			const float s1 = sin(info.rotation.y);

			instance_t instance{};

			instance.transform[0]  = info.scale.x * (c1 * c3 + s1 * s2 * s3);
			instance.transform[1]  = info.scale.x * (c2 * s3);
			instance.transform[2]  = info.scale.x * (c1 * s2 * s3 - c3 * s1);
			instance.transform[3]  = 0.0f;

			instance.transform[4]  = info.scale.y * (c3 * s1 * s2 - c1 * s3);
			instance.transform[5]  = info.scale.y * (c2 * c3);
			instance.transform[6]  = info.scale.y * (c1 * c3 * s2 + s1 * s3);
			instance.transform[7]  = 0.0f;
			
			instance.transform[8]  = info.scale.z * (c2 * s1);
			instance.transform[9]  = info.scale.z * (-s2);
			instance.transform[10] = info.scale.z * (c1 * c2);
			instance.transform[11] = 0.0f;

			instance.transform[12] = info.position.x;
			instance.transform[13] = info.position.y;
			instance.transform[14] = info.position.z;
			instance.transform[15] = 1.0f;

			registry->total_instance_count++;

			memcpy(
				(uint8_t*)local_graphics_allocator->instance_buffer.info.pMappedData + registry->total_instance_count * sizeof(instance_t),
				&instance, sizeof(instance_t));

			//LOG_INFO(TAG_OLIVIA, "instance_batch: add instance %u. entity_count: %u", registry->entity_count, registry->entity_count + 1);
		}

		//LOG_INFO(TAG_OLIVIA, "registry: create entity %u. entity_count: %u", registry->entity_count, registry->entity_count + 1);
		return registry->entity_count++;
	}

	uint32_t request_instance_batch(registry registry, cmesh_t mesh)
	{
		for (uint32_t i = 0; i < registry->batch_count; ++i)
		{
			if (registry->instance_mesh[i] == mesh)
			{
				LOG_INFO(TAG_OLIVIA, "registry: request instance batch %u of mesh %u", i, mesh);
				return i;
			}
		}
		
		LOG_INFO(TAG_OLIVIA, "registry: create instance batch %u of mesh %u", registry->batch_count, mesh);
		return registry->batch_count++;
	}


	void render_system()
	{
		registry_t& registry = *local_current_registry;

		// vkCmdBindDescriptorSet

		// vkCmdBindPipeline

		// vkCmdBindVertexBuffers
		// vkCmdBindIndexBuffer

		for (uint32_t i = 0; i < registry.batch_count; ++i)
		{
			// drawIndexed
		}
	}
}