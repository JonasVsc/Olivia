#pragma once
#include "Olivia_Registry.h"

namespace Olivia
{
	inline void InputSystem(entt::registry& registry, const SDL_Event& event)
	{
		auto& input = registry.ctx().get<InputState>();

		if (event.type == SDL_EVENT_KEY_DOWN)
			input.keys[event.key.scancode] = true;
		if (event.type == SDL_EVENT_KEY_UP)
			input.keys[event.key.scancode] = false;
	}

	inline void MovementSystem(entt::registry& registry, float dt)
	{
		auto& input = registry.ctx().get<InputState>();
		auto view = registry.view<CPosition>();

		for (auto [entity, pos] : view.each())
		{
			if (input.keys[SDL_SCANCODE_W]) pos.y += 100.0f * dt;
			if (input.keys[SDL_SCANCODE_S]) pos.y -= 100.0f * dt;
			if (input.keys[SDL_SCANCODE_A]) pos.x -= 100.0f * dt;
			if (input.keys[SDL_SCANCODE_D]) pos.x += 100.0f * dt;
		}
	}

	inline void RenderSystem(entt::registry& registry)
	{
		auto view = registry.view<CPosition, CMesh>();

		for (auto [entity, pos, mesh] : view.each())
		{
			LOG_INFO(TAG_OLIVIA, "Rendering Entity %d -> Mesh(%d), Position(%.1f, %.1f, %.1f)\n", entity, mesh, pos.x, pos.y, pos.z);
		}
	}
}