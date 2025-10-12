#version 460

struct MeshData
{
	uint vertex_buffer_offset;
	uint index_buffer_offset;
	uint index_count;
};

// Vertex
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inColor;

// Instance
layout (location = 4) in mat4 inTransform;
layout (location = 8) in uint inMeshIndex;

// Out
layout(location = 0) out vec4 outColor;
layout(location = 1) out uint outMeshIndex;

void main()
{

	gl_Position = vec4(inPosition, 1.0);
	
	outColor = inColor;
	outMeshIndex = inMeshIndex;
}