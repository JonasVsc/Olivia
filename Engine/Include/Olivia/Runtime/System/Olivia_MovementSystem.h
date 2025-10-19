#pragma once
#include "Olivia/Olivia_Runtime.h"

namespace Olivia
{
	class MovementSystem
	{
	public:

		MovementSystem(Registry& registry);

		void update();

	private:

		Registry& m_registry;


	};
} // Olivia