#pragma once
#include "Olivia/Olivia_Core.h"
#include "Olivia/Olivia_Graphics.h"

#include "Olivia_Component.h"

namespace Olivia
{
	constexpr uint32_t MAX_ENTITIES{ 10 };

	class Registry
	{
	public:

		Registry();
		Registry(const Registry&) = delete;
		~Registry();

	private:

		CMesh mesh[MAX_ENTITIES]{};
		CTransform transform[MAX_ENTITIES]{};
	};

} // Olivia