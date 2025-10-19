#version 460

// Vertex
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

// Instance
layout (location = 3) in mat4 inTransform;

void main()
{
	gl_Position = inTransform * vec4(inPosition, 1.0);
}