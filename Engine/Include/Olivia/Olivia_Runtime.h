#pragma once
#include "olivia/olivia_core.h"
#include "olivia/olivia_graphics.h"
#include "runtime/olivia_components.h"

namespace olivia
{
	OLIVIA_DEFINE_HANDLE(registry);

	using entity_t = uint32_t;

	struct entity_info_t
	{
		cmesh_t     mesh;
		cposition_t position;
		crotation_t rotation;
		cscale_t    scale;
	};

	registry create_registry();

	void bind_registry(registry registry);

	entity_t create_entity(registry registry, const entity_info_t& info);

	// system

	void render_system();
}