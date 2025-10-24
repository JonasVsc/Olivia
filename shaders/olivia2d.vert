#version 460

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inNormal;

layout (set = 0, binding = 0) uniform UniformBufferObj {
	mat4 projection;
	mat4 view;
} ubo;

struct InstanceData
{
	mat4 transform;
	vec4 color;
};

// STORAGE BUFFER - contains array of instance data
layout (set = 0, binding = 1) readonly buffer StorageBufferObj {
    InstanceData data[];  // Array of transforms for each instance
} instance;

layout(location = 0) out vec4 outColor;

void main()
{
	mat4 instanceTransform = instance.data[gl_InstanceIndex].transform;

	outColor = instance.data[gl_InstanceIndex].color;

	gl_Position = ubo.projection * ubo.view * instanceTransform * vec4(inPosition, 0.0, 1.0);
}