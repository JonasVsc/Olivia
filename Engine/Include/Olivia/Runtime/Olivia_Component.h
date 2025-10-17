#pragma once
#include <glm/glm.hpp>
#include "Olivia/Olivia_Graphics.h"

namespace Olivia
{

	using CMesh = MeshInfo;

	struct CTransform 
	{
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
	};

} // Olivia