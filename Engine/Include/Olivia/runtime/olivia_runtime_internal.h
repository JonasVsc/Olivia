#pragma once
#include "olivia/olivia_runtime.h"
#include "olivia/graphics/olivia_graphics_internal.h"
#include "olivia_components.h"

namespace olivia
{
	constexpr uint32_t MAX_ENTITIES{ 10'000 };
	constexpr uint32_t MAX_BATCHES{ 150 };

	struct instance_batch_t
	{

	};

	struct registry_t
	{
		renderer renderer;

		// entities

		cmesh_t      cmesh_pool[MAX_ENTITIES];
		cposition_t  cposition_pool[MAX_ENTITIES];
		crotation_t  crotation_pool[MAX_ENTITIES];
		cscale_t     cscale_pool[MAX_ENTITIES];

		uint32_t entity_count;

		// instances

		bool     batch_initialized[MAX_BATCHES];
		cmesh_t  instance_mesh[MAX_BATCHES];
		uint32_t instance_offset[MAX_BATCHES];
		uint32_t instance_count[MAX_BATCHES];
		
		uint32_t previous_instance_offset;
		uint32_t total_instance_count;
		uint32_t batch_count;
	};

	uint32_t request_instance_batch(registry registry, cmesh_t mesh);
}