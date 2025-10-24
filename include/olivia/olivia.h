#pragma once
#include "olivia_types.h"
#include "olivia/olivia_math.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <cstdio>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define KILOBYTES(x) ((x) * 1024ull)
#define MEGABYTES(x) (KILOBYTES(x) * 1024ull)
#define GIGABYTES(x) (MEGABYTES(x) * 1024ull)

#define VK_CHECK(x)														\
	do																	\
	{																	\
		VkResult err = x;												\
		if (err)														\
		{																\
			printf("Detected Vulkan error: %d", err);					\
			abort();													\
		}																\
	} while (0)


constexpr uint32_t MAX_FRAMES{ 2 };
constexpr uint32_t MAX_INSTANCES{ 10'000 };

enum PipelineType
{
	PIPELINE_2D,
	PIPELINE_3D,
	PIPELINE_MAX
};

enum MeshType
{
	MESH_QUAD,
	MESH_MAX
};

struct context_t
{
	// --- platform ---

	SDL_Window*        window;
	float              window_width;
	float              window_height;
	SDL_Event          event;
	bool               current_keys[SDL_SCANCODE_COUNT]{};
	bool               previous_keys[SDL_SCANCODE_COUNT]{};
	float              delta_time;
	// --- vulkan core ---

	VkInstance         instance;
	VkSurfaceKHR       surface;
	VkPhysicalDevice   gpu;
	VkQueue            queue;
	uint32_t           graphics_queue_index;
	VkDevice           device;
	VmaAllocator       allocator;
	VkCommandPool      command_pool;
	VkCommandBuffer    command_buffers[5];
	VkSwapchainKHR     swapchain;
	uint32_t           swapchain_size;
	VkExtent2D         swapchain_extent;
	VkSurfaceFormatKHR swapchain_format;
	VkImage            swapchain_images[5];
	VkImageView        swapchain_views[5];
	VkFence            queue_submit[MAX_FRAMES];
	VkSemaphore        acquire_image[MAX_FRAMES];
	VkSemaphore        present_image[MAX_FRAMES];
	uint32_t           current_frame;
	uint32_t           image_index;

	// --- camera data ---

	buffer_t       uniform_buffer[MAX_FRAMES];
	uniform_data_t camera_data[MAX_FRAMES];
	vec3_t         camera_position;
	vec3_t         camera_rotation;
	float          camera_zoom;
	float          camera_left;
	float          camera_right;
	float          camera_top;
	float          camera_bottom;

	// --- instances ---
	buffer_t     instances_buffer[MAX_FRAMES];
	instance3d_t instances[MAX_INSTANCES];
	uint32_t     instance_count;
	// --- graphics resources ---

	VkDescriptorPool      descriptor_pool;
	VkDescriptorSetLayout set_layout;
	VkDescriptorSet       set[MAX_FRAMES];
	
	pipeline_t pipelines[PIPELINE_MAX];
	mesh_t     meshes[MESH_MAX];

};

extern context_t* g_context;

// --- helpers ---

void init_vulkan_core();
void init_buffers();
void init_descriptors();
void init_pipelines();
void init_meshes();
void init_scene();
void begin_frame();
void end_frame();

inline bool is_key_pressed(SDL_Scancode code)
{
	return (code >= 0 && code < 256) ? (g_context->current_keys[code] && !g_context->previous_keys[code]) : false;
}

inline bool is_key_down(SDL_Scancode code)
{
	return (code >= 0 && code < 256) ? g_context->current_keys[code] : false;
}

inline void update_input_state()
{
	memcpy(g_context->previous_keys, g_context->current_keys, sizeof(g_context->current_keys));
}

// --- internal ---

buffer_t create_buffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size);
VkShaderModule create_shader_module(VkDevice device, const char* shader_path);
pipeline_t build_pipeline(const pipeline_info_t& info);
void recreate_swapchain();