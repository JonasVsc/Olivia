#pragma once
#include "olivia_types.h"
#include <glm/glm.hpp>

// --- projections

inline mat4_t ortho_projection(float left, float right, float bottom, float top, float near, float far)
{
	return {
		2.0f / (right - left),            0.0f,                              0.0f,                        0.0f,
		0.0f,                             2.0f / (top - bottom),             0.0f,                        0.0f,
		0.0f,                             0.0f,                              1.0f / (far - near),         0.0f,
		-(right + left) / (right - left), -(top + bottom) / (top - bottom), -near / (far - near),         1.0f,
	};
}

// --- mat identities ---

inline mat3_t mat3_identity()
{
	return {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	};
}

inline mat4_t mat4_identity()
{
	return {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
}

// --- transformations ---

inline void mat4_translate(mat4_t& src, const vec3_t& pos)
{
	src.data[12]  = pos.x;
	src.data[13]  = pos.y;
	src.data[14] =  pos.z;
}

inline void mat4_scale(mat4_t& src, const vec3_t& scale)
{
	src.data[0] = scale.x;
	src.data[5] = scale.y;
	src.data[10] = scale.z;
}
