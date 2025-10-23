#version 460

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inNormal;

layout(location = 2) in mat3 inTransform;

void main()
{

	gl_Position = vec4(inPosition, 0.0, 1.0);

}