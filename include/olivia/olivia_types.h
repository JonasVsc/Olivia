#pragma once
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

// --- math ---

struct vec2_t { float x, y; };
struct vec3_t { float x, y, z; };
struct vec4_t { float x, y, z, w; };

struct mat3_t { float data[9]; };
struct mat4_t { float data[16]; };

// --- layout ---

struct vertex2d_t 
{ 
	vec2_t position;
	vec2_t normal;
};
struct vertex3d_t 
{ 
	vec3_t position;
	vec3_t normal;
	vec2_t uv;
};

struct instance2d_t 
{ 
	mat3_t transform; 
};

struct instance3d_t
{
	mat4_t transform;
	vec4_t color;
};

// --- descriptor uniforms ---

struct uniform_data_t
{
	mat4_t view;
	mat4_t projection;
};

// --- graphics primitives ---

struct buffer_t
{
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo info;
};

// --- graphics resources --- 

struct pipeline_t
{
	VkPipeline       pipeline;
	VkPipelineLayout layout;
};

struct mesh_t
{
	buffer_t vertex_buffer;
	buffer_t index_buffer;
	uint32_t index_count;
};

// --- helpers, infos ---

struct pipeline_info_t
{
	const char* vertex_shader_path;
	const char* frag_shader_path;
	uint32_t vertex_binding_count;
	VkVertexInputBindingDescription* vertex_bindings;
	uint32_t vertex_attribute_count;
	VkVertexInputAttributeDescription* vertex_attributes;
	uint32_t set_layout_count;
	VkDescriptorSetLayout* set_layouts;
};