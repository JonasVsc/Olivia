#include "Olivia/Olivia_Renderer.h"
#include "Olivia/Olivia_Math.h"

#include "Olivia/Systems/Olivia_RenderSystem.h"

#include <vulkan/vulkan.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include <cstdio>
#include <cstdlib>

#ifdef OLIVIA_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

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

namespace Olivia
{
	constexpr uint32_t MESH_ATLAS_SIZE{ MEGABYTES(100) };
	
	constexpr uint32_t MAX_MESH_ATLAS{ 1 };

	constexpr uint32_t MAX_CONCURRENT_FRAMES{ 2 };
	
	constexpr uint32_t MAX_PIPELINES{ 10 };

	constexpr uint32_t MAX_MESHES{ 10 };
	
	constexpr uint32_t MAX_INDIRECT_CMDS{ MAX_MESHES };

	constexpr uint32_t MAX_INSTANCES{ 10'000 };

	struct Buffer
	{
		VkBuffer handle;
		VmaAllocation allocation;
		VmaAllocationInfo info;
	};

	struct VulkanContext
	{
		Window window;
		VkInstance instance;
		VkSurfaceKHR surface;
		VkPhysicalDevice gpu;
		VkQueue queue;
		uint32_t graphics_queue_index;
		VkDevice device;
		VmaAllocator allocator;
		VkCommandPool command_pool;
		VkCommandBuffer command_buffers[5];
		VkSwapchainKHR swapchain;
		uint32_t swapchain_size;
		VkExtent2D swapchain_extent;
		VkImage swapchain_images[5];
		VkImageView swapchain_views[5];
		VkSurfaceFormatKHR swapchain_format;
		VkFence queue_submit[MAX_CONCURRENT_FRAMES];
		VkSemaphore acquire_image[MAX_CONCURRENT_FRAMES];
		VkSemaphore present_image[5];
		uint32_t current_frame;
		uint32_t image_index;

		VkCommandBuffer imm_command_buffer;
		VkFence imm_fence;
	};

	struct UniformData
	{
		float projection[16];
		float view[16];
	};

	struct GraphicsPipeline_T
	{
		VkPipeline handle;
		VkPipelineLayout layout;
	};

	struct MeshData
	{
		uint32_t vertex_buffer_offset;
		uint32_t index_buffer_offset;
		uint32_t index_count;
	};

	struct MeshAtlas
	{
		Buffer vertex_buffer;
		Buffer index_buffer;
		uint32_t mesh_count;
		uint32_t bytes_used;

		MeshData meshes[MAX_MESHES];
	};

	struct InstanceData
	{
		float transform[16];
		Mesh mesh;
	};

	struct InstanceAtlas
	{
		Buffer buffer;
		uint32_t instance_count;
		InstanceData instances[MAX_INSTANCES];
	};

	struct Renderer_T
	{
		VulkanContext ctx;
	
		// view, projection
		Buffer uniform_buffer[MAX_CONCURRENT_FRAMES];
		GraphicsPipeline current_pipeline;

		VkDescriptorPool descriptor_pool;
		VkDescriptorSetLayout descriptor_set_layout;
		VkDescriptorSet descriptor_set[MAX_CONCURRENT_FRAMES];

		Buffer indirect_buffer;
		VkDrawIndexedIndirectCommand indirect_cmd[MAX_INDIRECT_CMDS];
		uint32_t indirect_cmd_hash[MAX_INDIRECT_CMDS];
		uint32_t indirect_cmd_created;
		uint32_t previous_cmd;

		GraphicsPipeline_T pipelines[MAX_PIPELINES];
		uint32_t pipeline_count;

		MeshAtlas mesh_atlas;

		InstanceAtlas instance_atlas;

		UniformData uniform_data; // view, projection
	};

	static Renderer g_renderer{ nullptr };

	void CreateDevice(VulkanContext* ctx)
	{
		// Create VkInstance
		{
			VkApplicationInfo application_info
			{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Olivia",
				.pEngineName = "Olivia",
				.apiVersion = VK_API_VERSION_1_3
			};

			const char* extensions[]
			{
				VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef OLIVIA_WIN32
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif // OLIVIA_WIN32
			};

			VkInstanceCreateInfo instance_info
			{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pApplicationInfo = &application_info,
				.enabledExtensionCount = ARRAY_SIZE(extensions),
				.ppEnabledExtensionNames = extensions
			};

#ifdef OLIVIA_DEBUG
			const char* layers[]
			{
				"VK_LAYER_KHRONOS_validation"
			};

			instance_info.enabledLayerCount = ARRAY_SIZE(layers);
			instance_info.ppEnabledLayerNames = layers;
#endif // OLIVIA_DEBUG

			VK_CHECK(vkCreateInstance(&instance_info, nullptr, &ctx->instance));
		}

		// Create VkSurfaceKHR
		{
			if (!WindowCreateSurface(ctx->instance, ctx->window, (void**)&ctx->surface))
			{
				printf("Failed to create vulkan window surface\n");
				abort();
			}
		}

		// Create VkDevice
		{
			uint32_t gpu_count{};
			vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, nullptr);

			if (gpu_count <= 0)
			{

				printf("No suitable gpu found\n");
				abort();
			}

			VkPhysicalDevice gpus[10]{};
			vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, gpus);

			for (uint32_t i = 0; i < gpu_count; ++i)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpus[i], &properties);

				if (properties.apiVersion < VK_API_VERSION_1_3)
				{
					continue;
				}

				uint32_t queue_family_count = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queue_family_count, nullptr);

				VkQueueFamilyProperties queue_family_properties[20];
				vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queue_family_count, queue_family_properties);

				ctx->graphics_queue_index = UINT32_MAX;

				for (uint32_t i = 0; i < queue_family_count; ++i)
				{
					VkBool32 support_presentation{ VK_FALSE };
					vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], i, ctx->surface, &support_presentation);

					if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && support_presentation)
					{
						ctx->graphics_queue_index = i;
						ctx->gpu = gpus[i];
						break;
					}
				}
			}

			if (ctx->graphics_queue_index == UINT32_MAX)
			{
				abort();
			}

			const char* extensions[]
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
			};

			VkPhysicalDeviceVulkan13Features features
			{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
				.pNext = nullptr,
				.dynamicRendering = VK_TRUE
			};

			float queue_priority{ 1.0f };
			VkDeviceQueueCreateInfo queue_info[]
			{
				// Graphics Queue
				{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = ctx->graphics_queue_index,
					.queueCount = 1,
					.pQueuePriorities = &queue_priority
				}
			};

			VkDeviceCreateInfo device_info
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = &features,
				.queueCreateInfoCount = ARRAY_SIZE(queue_info),
				.pQueueCreateInfos = queue_info,
				.enabledExtensionCount = ARRAY_SIZE(extensions),
				.ppEnabledExtensionNames = extensions
			};

			VK_CHECK(vkCreateDevice(ctx->gpu, &device_info, nullptr, &ctx->device));

			vkGetDeviceQueue(ctx->device, ctx->graphics_queue_index, 0, &ctx->queue);
		}

		// Create VmaAllocator
		{
			VmaAllocatorCreateInfo vma_info
			{
				.physicalDevice = ctx->gpu,
				.device = ctx->device,
				.instance = ctx->instance,
				.vulkanApiVersion = VK_API_VERSION_1_3
			};

			VK_CHECK(vmaCreateAllocator(&vma_info, &ctx->allocator));
		}

		// Create VkCommandPool
		{
			VkCommandPoolCreateInfo command_pool_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = ctx->graphics_queue_index
			};

			VK_CHECK(vkCreateCommandPool(ctx->device, &command_pool_info, nullptr, &ctx->command_pool));
		}
	}
	void CreateSwapchain(VulkanContext* ctx)
	{
		// Create VkSwapchainKHR
		{
			VkSurfaceFormatKHR preferred_formats[]
			{
				{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
			};

			uint32_t format_count{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->gpu, ctx->surface, &format_count, nullptr);
			VkSurfaceFormatKHR formats[15]{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->gpu, ctx->surface, &format_count, formats);

			for (uint32_t i = 0; i < format_count; ++i)
			{
				for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
				{
					if (preferred_formats[j].format == formats[i].format &&
						preferred_formats[j].colorSpace == formats[i].colorSpace)
					{
						ctx->swapchain_format = preferred_formats[j];
						break;
					}
				}

				if (ctx->swapchain_format.format != VK_FORMAT_UNDEFINED)
				{
					break;
				}
			}

			if (ctx->swapchain_format.format == VK_FORMAT_UNDEFINED)
			{
				ctx->swapchain_format = formats[0];
			}

			VkSurfaceCapabilitiesKHR capabilities{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->gpu, ctx->surface, &capabilities);

			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				ctx->swapchain_extent = capabilities.currentExtent;
			}
			else
			{
				uint32_t width{ 0 }, height{ 0 };
				GetWindowSize(ctx->window, &width, &height);

				ctx->swapchain_extent.width = Clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				ctx->swapchain_extent.height = Clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			}

			uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

			VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

			VkSwapchainCreateInfoKHR swapchain_info
			{
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = ctx->surface,
				.minImageCount = min_image_count,
				.imageFormat = ctx->swapchain_format.format,
				.imageColorSpace = ctx->swapchain_format.colorSpace,
				.imageExtent = ctx->swapchain_extent,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.preTransform = capabilities.currentTransform,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = presentation_mode,
				.oldSwapchain = VK_NULL_HANDLE,
			};

			VK_CHECK(vkCreateSwapchainKHR(ctx->device, &swapchain_info, nullptr, &ctx->swapchain));
		}

		// Swapchain VkImages
		{
			vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_size, nullptr);
			vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_size, ctx->swapchain_images);
		}

		// Swapchain VkImageViews
		{
			for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
			{
				VkImageViewCreateInfo image_view_info
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = ctx->swapchain_images[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = ctx->swapchain_format.format,
					.components
					{
						VK_COMPONENT_SWIZZLE_IDENTITY,
						VK_COMPONENT_SWIZZLE_IDENTITY,
						VK_COMPONENT_SWIZZLE_IDENTITY,
						VK_COMPONENT_SWIZZLE_IDENTITY
					},
					.subresourceRange
					{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1
					}
				};

				VK_CHECK(vkCreateImageView(ctx->device, &image_view_info, nullptr, &ctx->swapchain_views[i]));
			}
		}
	}

	void CreateSyncObjects(VulkanContext* ctx)
	{
		// Allocate VkCommandBuffers
		{
			VkCommandBufferAllocateInfo command_buffer_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = ctx->command_pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = MAX_CONCURRENT_FRAMES
			};

			VK_CHECK(vkAllocateCommandBuffers(ctx->device, &command_buffer_info, ctx->command_buffers));

			command_buffer_info.commandBufferCount = 1;
			
			VK_CHECK(vkAllocateCommandBuffers(ctx->device, &command_buffer_info, &ctx->imm_command_buffer));
		}

		// Create VkFence, VkSemaphore
		{
			VkSemaphoreCreateInfo semaphore_info
			{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
			};

			VkFenceCreateInfo fence_info
			{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT
			};


			for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
			{
				VK_CHECK(vkCreateSemaphore(ctx->device, &semaphore_info, nullptr, &ctx->acquire_image[i]));
				VK_CHECK(vkCreateFence(ctx->device, &fence_info, nullptr, &ctx->queue_submit[i]));
			}

			for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
			{
				VK_CHECK(vkCreateSemaphore(ctx->device, &semaphore_info, nullptr, &ctx->present_image[i]));
			}

			VK_CHECK(vkCreateFence(ctx->device, &fence_info, nullptr, &ctx->imm_fence));
				
		}
	}

	Buffer CreateBuffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size)
	{
		VkBufferCreateInfo buffer_create_info
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = (VkDeviceSize)size,
			.usage = usage_flags
		};

		VmaAllocationCreateInfo allocation_create_info
		{
			.flags = allocation_flags,
			.usage = memory_usage,
		};

		Buffer b{};
		VK_CHECK(vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &b.handle, &b.allocation, &b.info));

		printf("[RENDERER] Requested: [%zu] bytes\n", size);

		return b;
	}

	void PrepareBuffers(Renderer r)
	{
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			// View Projection
			r->uniform_buffer[i] = CreateBuffer(
				r->ctx.allocator,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VMA_MEMORY_USAGE_AUTO,
				VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
				sizeof(UniformData));
		}

		// Mesh Atlas
		r->mesh_atlas.vertex_buffer = CreateBuffer(
			r->ctx.allocator,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			MESH_ATLAS_SIZE);

		r->mesh_atlas.index_buffer = CreateBuffer(
			r->ctx.allocator,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			MESH_ATLAS_SIZE);
	}

	void CreateDescriptors(Renderer r)
	{
		// CREATE DESCRIPTOR POOL

		VkDescriptorPoolSize pool_sizes[]
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_CONCURRENT_FRAMES },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_CONCURRENT_FRAMES },
		};

		VkDescriptorPoolCreateInfo descriptor_pool_info
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = MAX_CONCURRENT_FRAMES,
			.poolSizeCount = ARRAY_SIZE(pool_sizes),
			.pPoolSizes = pool_sizes
		};

		VK_CHECK(vkCreateDescriptorPool(r->ctx.device, &descriptor_pool_info, nullptr, &r->descriptor_pool));

		// DESCRIPTOR SET LAYOUT

		VkDescriptorSetLayoutBinding set_layout_bindings[]
		{
			// View, Projection
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },

			// Mesh Atlas
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },
		};

		VkDescriptorSetLayoutCreateInfo set_layout_info
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = ARRAY_SIZE(set_layout_bindings),
			.pBindings = set_layout_bindings
		};

		VK_CHECK(vkCreateDescriptorSetLayout(r->ctx.device, &set_layout_info, nullptr, &r->descriptor_set_layout));

		// DESCRIPTOR SETS

		VkDescriptorSetAllocateInfo set_allocate_info
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = r->descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &r->descriptor_set_layout,
		};

		VkDescriptorBufferInfo mesh_atlas_info{ r->mesh_atlas.buffer.handle, 0, VK_WHOLE_SIZE };

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			VK_CHECK(vkAllocateDescriptorSets(r->ctx.device, &set_allocate_info, &r->descriptor_set[i]));

			VkDescriptorBufferInfo uniform_info{ r->uniform_buffer[i].handle, 0, sizeof(UniformData) };

			VkWriteDescriptorSet write_sets[]
			{
				// Binding 0: Vertex shader uniform buffer (view, projection)
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = r->descriptor_set[i],
					.dstBinding = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &uniform_info
				},

				// Binding 1: Vertex shader mesh atlas storage buffer
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = r->descriptor_set[i],
					.dstBinding = 1,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.pBufferInfo = &mesh_atlas_info
				}
			};

			vkUpdateDescriptorSets(r->ctx.device, ARRAY_SIZE(write_sets), write_sets, 0, nullptr);
		}
	}

	void RecreateSwapchain(VulkanContext* ctx)
	{
		vkDeviceWaitIdle(ctx->device);

		// DESTROY OLD SWAPCHAIN & SYNC OBJECTS

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vkDestroySemaphore(ctx->device, ctx->acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
		{
			vkDestroySemaphore(ctx->device, ctx->present_image[i], nullptr);
			vkDestroyFence(ctx->device, ctx->queue_submit[i], nullptr);
		}

		for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
		{
			vkDestroyImageView(ctx->device, ctx->swapchain_views[i], nullptr);
		}

		vkDestroySwapchainKHR(ctx->device, ctx->swapchain, nullptr);

		ctx->current_frame = 0;

		// RECREATE SYCHRONIZATION OBJECTS
		VkSemaphoreCreateInfo semaphore_info
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};

		VkFenceCreateInfo fence_info
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			VK_CHECK(vkCreateSemaphore(ctx->device, &semaphore_info, nullptr, &ctx->acquire_image[i]));
			VK_CHECK(vkCreateFence(ctx->device, &fence_info, nullptr, &ctx->queue_submit[i]));
		}

		for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
		{
			VK_CHECK(vkCreateSemaphore(ctx->device, &semaphore_info, nullptr, &ctx->present_image[i]));
		}

		// RECREATE SWAPCHAIN
		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->gpu, ctx->surface, &capabilities);

		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			ctx->swapchain_extent = capabilities.currentExtent;
		}
		else
		{
			uint32_t width{ 0 }, height{ 0 };
			GetWindowSize(ctx->window, &width, &height);

			ctx->swapchain_extent.width = Clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			ctx->swapchain_extent.height = Clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}

		uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

		VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

		VkSwapchainCreateInfoKHR swapchain_info
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = ctx->surface,
			.minImageCount = min_image_count,
			.imageFormat = ctx->swapchain_format.format,
			.imageColorSpace = ctx->swapchain_format.colorSpace,
			.imageExtent = ctx->swapchain_extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentation_mode,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		VK_CHECK(vkCreateSwapchainKHR(ctx->device, &swapchain_info, nullptr, &ctx->swapchain));

		vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_size, nullptr);
		vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_size, ctx->swapchain_images);

		for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
		{
			VkImageViewCreateInfo image_view_info
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = ctx->swapchain_images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = ctx->swapchain_format.format,
				.components
				{
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY
				},
				.subresourceRange
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			VK_CHECK(vkCreateImageView(ctx->device, &image_view_info, nullptr, &ctx->swapchain_views[i]));
		}
	}

	VkShaderModule CreateShaderModule(VkDevice device, const char* shader_path)
	{
		size_t code_size;
		uint8_t* shader_code = (uint8_t*)ReadFileR(shader_path, &code_size);

		VkShaderModuleCreateInfo shader_info
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = code_size,
			.pCode = (const uint32_t*)shader_code
		};

		VkShaderModule shader_module{ VK_NULL_HANDLE };
		VK_CHECK(vkCreateShaderModule(device, &shader_info, nullptr, &shader_module));

		FreeFileR(shader_code);
		return shader_module;
	}

	Renderer CreateRenderer(Allocator allocator, Window window)
	{
		Renderer r = (Renderer)Allocate(allocator, sizeof(Renderer_T));
		
		if (r)
		{
			g_renderer = r;

			r->ctx.window = window;

			CreateDevice(&r->ctx);
			
			CreateSwapchain(&r->ctx);

			CreateSyncObjects(&r->ctx);

			PrepareBuffers(r);

			CreateDescriptors(r);

			memset(r->indirect_cmd_hash, UINT32_MAX, sizeof(r->indirect_cmd_hash));
		}

		return r;
	}

	Renderer GetRenderer()
	{
		return g_renderer;
	}

	void DestroyRenderer(Renderer r)
	{
		assert(r && "Invalid pointer");

		VulkanContext* ctx = &r->ctx;

		vkDeviceWaitIdle(ctx->device);

		// DESTROY UNIFORM BUFFER

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vmaDestroyBuffer(ctx->allocator, r->uniform_buffer[i].handle, r->uniform_buffer[i].allocation);
		}

		// DESTROY INDIRECT BUFFER

		vmaDestroyBuffer(ctx->allocator, r->indirect_buffer.handle, r->indirect_buffer.allocation);

		// DESTROY MESH ATLAS

		vmaDestroyBuffer(ctx->allocator, r->mesh_atlas.buffer.handle, r->mesh_atlas.buffer.allocation);

		// DESTROY INSTANCE ATLAS

		vmaDestroyBuffer(ctx->allocator, r->instance_atlas.buffer.handle, r->instance_atlas.buffer.allocation);

		// DESTROY DESCRIPTOR

		vkDestroyDescriptorSetLayout(ctx->device, r->descriptor_set_layout, nullptr);
		vkDestroyDescriptorPool(ctx->device, r->descriptor_pool, nullptr);

		// DESTROY PIPELINES

		for (uint32_t i = 0; i < r->pipeline_count; ++i)
		{
			vkDestroyPipelineLayout(ctx->device, r->pipelines[i].layout, nullptr);
			vkDestroyPipeline(ctx->device, r->pipelines[i].handle, nullptr);
		}

		// DESTROY CONTEXT

		vkDestroyFence(ctx->device, ctx->imm_fence, nullptr);

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vkDestroySemaphore(ctx->device, ctx->acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
		{
			vkDestroyImageView(ctx->device, ctx->swapchain_views[i], nullptr);
			vkDestroySemaphore(ctx->device, ctx->present_image[i], nullptr);
			vkDestroyFence(ctx->device, ctx->queue_submit[i], nullptr);
		}

		vkDestroyCommandPool(ctx->device, ctx->command_pool, nullptr);
		vkDestroySwapchainKHR(ctx->device, ctx->swapchain, nullptr);
		vmaDestroyAllocator(ctx->allocator);

		vkDestroyDevice(ctx->device, nullptr);
		vkDestroySurfaceKHR(ctx->instance, ctx->surface, nullptr);
		vkDestroyInstance(ctx->instance, nullptr);
	}

	void BeginFrame(Renderer r)
	{
		vkWaitForFences(r->ctx.device, 1, &r->ctx.queue_submit[r->ctx.current_frame], VK_TRUE, UINT64_MAX);
		vkResetFences(r->ctx.device, 1, &r->ctx.queue_submit[r->ctx.current_frame]);

		VkResult acquire_result = vkAcquireNextImageKHR(r->ctx.device, r->ctx.swapchain, UINT64_MAX, r->ctx.acquire_image[r->ctx.current_frame], VK_NULL_HANDLE, &r->ctx.image_index);
		if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR)
		{
			printf("[OLIVIA] Recreating swapchain\n");
			RecreateSwapchain(&r->ctx);
			return;
		}
		else if (acquire_result != VK_SUCCESS)
		{
			printf("[OLIVIA] Failed to acquire next image!\n");
			abort();
		}

		vkResetCommandBuffer(r->ctx.command_buffers[r->ctx.current_frame], 0);
		VkCommandBufferBeginInfo cmd_begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(r->ctx.command_buffers[r->ctx.current_frame], &cmd_begin);

		VkImageMemoryBarrier image_barrier_write
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_PIPELINE_STAGE_NONE,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = r->ctx.swapchain_images[r->ctx.image_index],
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};

		vkCmdPipelineBarrier(
			r->ctx.command_buffers[r->ctx.current_frame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_write);

		VkRenderingAttachmentInfo color_attachment
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = r->ctx.swapchain_views[r->ctx.image_index],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {{0.0f, 0.0f, 0.0f, 0.0f}}
		};

		VkRect2D render_area
		{
			.offset = { 0 },
			.extent = r->ctx.swapchain_extent
		};

		VkRenderingInfo render_info
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = render_area,
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment,
		};

		vkCmdBeginRendering(r->ctx.command_buffers[r->ctx.current_frame], &render_info);

		VkViewport viewport
		{
			.width = (float)r->ctx.swapchain_extent.width,
			.height = (float)r->ctx.swapchain_extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D scissor
		{
			.offset = {0, 0},
			.extent = r->ctx.swapchain_extent
		};

		vkCmdSetViewport(r->ctx.command_buffers[r->ctx.current_frame], 0, 1, &viewport);
		vkCmdSetScissor(r->ctx.command_buffers[r->ctx.current_frame], 0, 1, &scissor);
	}

	void Draw(Renderer r)
	{
		VkDeviceSize offsets[]{ 0 };

		vkCmdBindVertexBuffers(
			r->ctx.command_buffers[r->ctx.current_frame],
			0,
			1, &r->mesh_atlas.buffer.handle,
			offsets);

		vkCmdBindIndexBuffer(
			r->ctx.command_buffers[r->ctx.current_frame],
			r->mesh_atlas.buffer.handle,
			0, 
			VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
			r->ctx.command_buffers[r->ctx.current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			r->current_pipeline->layout,
			0, 1, &r->descriptor_set[r->ctx.current_frame],
			0, nullptr);

		vkCmdDrawIndexedIndirect(
			r->ctx.command_buffers[r->ctx.current_frame],
			r->indirect_buffer.handle,
			0,
			r->indirect_cmd_created,
			sizeof(VkDrawIndexedIndirectCommand));
	}


	void EndFrame(Renderer r)
	{
		vkCmdEndRendering(r->ctx.command_buffers[r->ctx.current_frame]);

		VkImageMemoryBarrier image_barrier_present
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = r->ctx.swapchain_images[r->ctx.image_index],
			.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		};

		vkCmdPipelineBarrier(
			r->ctx.command_buffers[r->ctx.current_frame],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_present);

		vkEndCommandBuffer(r->ctx.command_buffers[r->ctx.current_frame]);

		VkPipelineStageFlags wait_stages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore wait_semaphores[]{ r->ctx.acquire_image[r->ctx.current_frame] };
		VkSemaphore signal_semaphores[]{ r->ctx.present_image[r->ctx.current_frame] };

		VkSubmitInfo submit_info
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = wait_semaphores,
			.pWaitDstStageMask = wait_stages,
			.commandBufferCount = 1,
			.pCommandBuffers = &r->ctx.command_buffers[r->ctx.current_frame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signal_semaphores
		};

		vkQueueSubmit(r->ctx.queue, 1, &submit_info, r->ctx.queue_submit[r->ctx.current_frame]);

		VkSwapchainKHR swapchains[]{ r->ctx.swapchain };

		VkPresentInfoKHR present_info
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signal_semaphores,
			.swapchainCount = 1,
			.pSwapchains = swapchains,
			.pImageIndices = &r->ctx.image_index
		};

		VkResult present_result = vkQueuePresentKHR(r->ctx.queue, &present_info);
		if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
		{
			printf("[OLIVIA] Recreating swapchain\n");
			RecreateSwapchain(&r->ctx);
			return;
		}
		else if (present_result != VK_SUCCESS)
		{
			printf("[OLIVIA] Failed to present image!\n");
			return;
		}

		r->ctx.current_frame = (r->ctx.current_frame + 1) % MAX_CONCURRENT_FRAMES;
	}

	GraphicsPipeline CreateGraphicsPipeline(Renderer renderer, const GraphicsPipelineCreateInfo* info)
	{
		VulkanContext* ctx = &renderer->ctx;

		VkShaderModule vert = CreateShaderModule(ctx->device, info->vertex_shader_path);
		VkShaderModule frag = CreateShaderModule(ctx->device, info->fragment_shader_path);

		VkPipelineShaderStageCreateInfo shader_stages[]
		{
			// Vert
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = vert,
				.pName = "main"
			},
			// Frag
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = frag,
				.pName = "main"
			}
		};

		VkVertexInputBindingDescription input_binding_description
		{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};

		VkVertexInputBindingDescription vertex_bindings[]
		{
			// Binding point 0: Mesh vertex layout description at per-vertex rate
			{
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			},

			// Binding point 1: Instanced data at per-instance rate
			{
				.binding = 1,
				.stride = sizeof(InstanceData),
				.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
			}
		};

		struct InstanceData
		{
			float transform[16];
			Mesh mesh;
		};

		VkVertexInputAttributeDescription vertex_attributes[]
		{
			// Per-vertex attributes
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
			{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
			{3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)},

			// Per-instance attributes
			{4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, transform[0])},
			{5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, transform[4])},
			{6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, transform[8])},
			{7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, transform[12])},

			// Mesh ID as instance attribute
			{8, 1, VK_FORMAT_R32_UINT, offsetof(InstanceData, mesh)},
		};

		VkPipelineVertexInputStateCreateInfo vertex_input_state_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = ARRAY_SIZE(vertex_bindings),
			.pVertexBindingDescriptions = vertex_bindings,
			.vertexAttributeDescriptionCount = ARRAY_SIZE(vertex_attributes),
			.pVertexAttributeDescriptions = vertex_attributes
		};

		VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		VkDynamicState dynamic_states[]
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = ARRAY_SIZE(dynamic_states),
			.pDynamicStates = dynamic_states
		};

		VkPipelineViewportStateCreateInfo viewport_state_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1
		};

		VkPipelineRasterizationStateCreateInfo rasterization_state_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.lineWidth = 1.0f
		};

		VkPipelineMultisampleStateCreateInfo multisample_state_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE
		};

		VkPipelineColorBlendAttachmentState color_blend_attachment_state
		{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		VkPipelineColorBlendStateCreateInfo color_blend_state_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &color_blend_attachment_state
		};

		VkPipelineRenderingCreateInfo pipeline_rendering_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &ctx->swapchain_format.format
		};

		VkPipelineLayoutCreateInfo layout_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &renderer->descriptor_set_layout
		};

		GraphicsPipeline pipeline = &renderer->pipelines[renderer->pipeline_count++];

		VK_CHECK(vkCreatePipelineLayout(ctx->device, &layout_info, nullptr, &pipeline->layout));

		VkGraphicsPipelineCreateInfo pipeline_info
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &pipeline_rendering_info,
			.stageCount = ARRAY_SIZE(shader_stages),
			.pStages = shader_stages,
			.pVertexInputState = &vertex_input_state_info,
			.pInputAssemblyState = &input_assembly_state_info,
			.pViewportState = &viewport_state_info,
			.pRasterizationState = &rasterization_state_info,
			.pMultisampleState = &multisample_state_info,
			.pColorBlendState = &color_blend_state_info,
			.pDynamicState = &dynamic_state_info,
			.layout = pipeline->layout,
			.renderPass = VK_NULL_HANDLE,
			.subpass = 0
		};

		vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline->handle);

		vkDestroyShaderModule(ctx->device, vert, nullptr);
		vkDestroyShaderModule(ctx->device, frag, nullptr);

		return pipeline;
	}

	void Bind(Renderer r, GraphicsPipeline pipeline)
	{
		vkCmdBindPipeline(
			r->ctx.command_buffers[r->ctx.current_frame], 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			pipeline->handle);

		r->current_pipeline = pipeline;
	}

	Mesh CreateMesh(Renderer renderer, const MeshCreateInfo* info)
	{
		MeshAtlas* atlas = &renderer->mesh_atlas;

		MeshData* mesh = &atlas->meshes[atlas->mesh_count];

		mesh->vertex_buffer_offset = atlas->bytes_used;
		mesh->index_buffer_offset = atlas->bytes_used + info->vertex_size;
		mesh->index_count = info->index_count;
		atlas->bytes_used += info->vertex_size + info->index_size;

		memcpy((char*)atlas->buffer.info.pMappedData + mesh->vertex_buffer_offset, info->vertex_data, info->vertex_size);
		memcpy((char*)atlas->buffer.info.pMappedData + mesh->index_buffer_offset, info->index_data, info->index_size);

		return atlas->mesh_count++;
	}

	Instance CreateInstance(Renderer r, const InstanceCreateInfo* info)
	{
		InstanceAtlas* atlas = &r->instance_atlas;
		
		if (atlas->instance_count == 0)
		{
			atlas->buffer = CreateBuffer(
				r->ctx.allocator,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VMA_MEMORY_USAGE_AUTO,
				VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
				sizeof(InstanceData) * MAX_INSTANCES);
		}

		InstanceData* instance = &atlas->instances[atlas->instance_count];
		memcpy(instance->transform, info->transform, sizeof(float) * 16);
		instance->mesh = info->mesh;

		size_t offset = atlas->instance_count * sizeof(InstanceData);
		memcpy((char*)atlas->buffer.info.pMappedData + offset, instance, sizeof(InstanceData));

		printf("[RENDERER] Cached Instance [%u]\n", atlas->instance_count);

		// Update Indirect CMD
		MeshData* mesh = &r->mesh_atlas.meshes[info->mesh];

		uint32_t selected_cmd{ 0 };
		for (uint32_t i = 0; i < r->indirect_cmd_created; ++i)
		{
			if (r->indirect_cmd_hash[i] == info->mesh)
			{
				selected_cmd = r->indirect_cmd_hash[i];
				break;
			}
		}

		if (r->indirect_cmd_created == 0)
		{
			r->indirect_cmd_hash[0] = info->mesh;
			r->indirect_cmd_created++;
		}

		r->indirect_cmd[selected_cmd].indexCount = mesh->index_count;
		r->indirect_cmd[selected_cmd].instanceCount += 1;
		r->indirect_cmd[selected_cmd].firstIndex = mesh->index_buffer_offset;
		r->indirect_cmd[selected_cmd].vertexOffset = 0;
		r->indirect_cmd[selected_cmd].firstInstance = r->indirect_cmd[r->previous_cmd].instanceCount - r->indirect_cmd[r->previous_cmd].firstInstance;
		
		r->previous_cmd = selected_cmd;

		printf("[INDIRECT CMD]\nStored VkDrawIndexedIndirectCommand [%u]\n"
			   "- FirstInstance: [%u]\n"
			   "- InstanceCount: [%u]\n"
			   "- FirstIndex:    [%u]\n"
			   "- VertexOffset:  [%u]\n"
			   "- IndexCount:    [%u]\n",
			   selected_cmd, 
			   r->indirect_cmd[selected_cmd].firstInstance, 
			   r->indirect_cmd[selected_cmd].instanceCount, 
			   r->indirect_cmd[selected_cmd].firstIndex,
			   r->indirect_cmd[selected_cmd].vertexOffset,
			   r->indirect_cmd[selected_cmd].indexCount);

		return atlas->instance_count++;
	}

	void BeginCmd(Renderer r)
	{
		VK_CHECK(vkResetFences(r->ctx.device, 1, &r->ctx.imm_fence));
		VK_CHECK(vkResetCommandBuffer(r->ctx.imm_command_buffer, 0));

		VkCommandBufferBeginInfo cmd_begin_info
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		VK_CHECK(vkBeginCommandBuffer(r->ctx.imm_command_buffer, &cmd_begin_info));
	}

	void EndCmd(Renderer r)
	{
		VkSubmitInfo submit_info
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &r->ctx.imm_command_buffer
		};

		VK_CHECK(vkEndCommandBuffer(r->ctx.imm_command_buffer));
		VK_CHECK(vkQueueSubmit(r->ctx.queue, 1, &submit_info, r->ctx.imm_fence));
		VK_CHECK(vkWaitForFences(r->ctx.device, 1, &r->ctx.imm_fence, VK_TRUE, UINT64_MAX));
	}

	void PrepareRender(Renderer r)
	{
		Buffer staging = CreateBuffer(
			r->ctx.allocator,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			r->indirect_cmd_created * sizeof(VkDrawIndexedIndirectCommand));

		if (staging.info.pMappedData)
		{
			memcpy(staging.info.pMappedData, r->indirect_cmd, r->indirect_cmd_created * sizeof(VkDrawIndexedIndirectCommand));
		}

		r->indirect_buffer = CreateBuffer(
			r->ctx.allocator,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			0,
			r->indirect_cmd_created * sizeof(VkDrawIndexedIndirectCommand));

		BeginCmd(r);

		VkBufferCopy buffer_copy{ 0, 0, r->indirect_cmd_created * sizeof(VkDrawIndexedIndirectCommand) };
		
		vkCmdCopyBuffer(
			r->ctx.imm_command_buffer,
			staging.handle,
			r->indirect_buffer.handle,
			1, &buffer_copy);

		EndCmd(r);

		vmaDestroyBuffer(r->ctx.allocator, staging.handle, staging.allocation);
	}


	void SetViewProjection(Renderer renderer, const ViewProjection* view_projection)
	{
		memcpy(
			renderer->uniform_buffer[renderer->ctx.current_frame].info.pMappedData, 
			view_projection, 
			sizeof(ViewProjection));
	}


} // Olivia