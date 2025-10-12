#include "Vulkan_Graphics.h"
#include "Olivia/Defines.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#ifdef OLIVIA_WIN32
#include "Platform/Platform.h"
#include <vulkan/vulkan_win32.h>
#endif // OLIVIA_WIN32

/// @brief Helper macro to test the result of Vulkan calls which can return an error.
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
	static Renderer* g_renderer = nullptr;

	bool CreateDevice(RendererContext* ctx)
	{
		// Create Instance
		{
			VkApplicationInfo application_info{};
			application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			application_info.apiVersion = VK_API_VERSION_1_3;
			application_info.pApplicationName = "Olivia";
			application_info.pEngineName = "Olivia";


			const char* extensions[]{
				VK_KHR_SURFACE_EXTENSION_NAME,
	#ifdef OLIVIA_WIN32
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	#endif // OLIVIA_WIN32
			};

			VkInstanceCreateInfo instance_create_info{};
			instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instance_create_info.pApplicationInfo = &application_info;
			instance_create_info.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
			instance_create_info.ppEnabledExtensionNames = extensions;

#ifdef OLIVIA_DEBUG
			uint32_t layer_count = 1;
			const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
			instance_create_info.enabledLayerCount = layer_count,
				instance_create_info.ppEnabledLayerNames = layers;
#endif // OLIVIA_DEBUG

			if (vkCreateInstance(&instance_create_info, nullptr, &ctx->instance) != VK_SUCCESS)
			{
				return false;
			}
		}

		// Create surface
		{
#ifdef OLIVIA_WIN32

			VkWin32SurfaceCreateInfoKHR win32_surface_create_info{};
			win32_surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			win32_surface_create_info.hinstance = GetModuleHandle(nullptr);
			win32_surface_create_info.hwnd = static_cast<HWND>(ctx->window_handle);

			if (vkCreateWin32SurfaceKHR(ctx->instance, &win32_surface_create_info, nullptr, &ctx->surface) != VK_SUCCESS)
			{
				return false;
			}

#endif // OLIVIA_WIN32
		}

		// Select GPU 
		{
			uint32_t gpu_count{};
			vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, nullptr);

			if (gpu_count <= 0)
				abort();

			VkPhysicalDevice gpus[10] = { 0 };
			vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, gpus);

			for (uint32_t i = 0; i < gpu_count; ++i)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpus[i], &properties);

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					ctx->gpu = gpus[i];
					break;
				}
			}
			if (ctx->gpu == VK_NULL_HANDLE)
			{
				ctx->gpu = gpus[0];
			}
		}

		// Create device
		{
			uint32_t gpu_count{};
			vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, nullptr);

			if (gpu_count < 1)
				abort();

			VkPhysicalDevice gpus[10] = { 0 };
			vkEnumeratePhysicalDevices(ctx->instance, &gpu_count, gpus);

			for (uint32_t i = 0; i < gpu_count; ++i)
			{
				VkPhysicalDeviceProperties device_properties;
				vkGetPhysicalDeviceProperties(gpus[i], &device_properties);

				if (device_properties.apiVersion < VK_API_VERSION_1_3)
				{
					continue;
				}

				uint32_t queue_family_count = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(ctx->gpu, &queue_family_count, nullptr);

				VkQueueFamilyProperties queue_family_properties[20] = { 0 };
				vkGetPhysicalDeviceQueueFamilyProperties(ctx->gpu, &queue_family_count, queue_family_properties);

				for (uint32_t i = 0; i < queue_family_count; ++i)
				{
					VkBool32 supports_present = VK_FALSE;
					vkGetPhysicalDeviceSurfaceSupportKHR(ctx->gpu, i, ctx->surface, &supports_present);

					if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present)
					{
						ctx->graphics_queue_index = i;
						break;
					}
				}

				if (ctx->graphics_queue_index >= 0)
				{
					ctx->gpu = gpus[i];
					break;
				}
			}

			if (ctx->graphics_queue_index < 0)
			{
				abort();
			}

			const char* extensions[]{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
				VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
			};


			VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features{};
			buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
			buffer_device_address_features.bufferDeviceAddress = VK_TRUE;
			buffer_device_address_features.pNext = nullptr;

			VkPhysicalDeviceVulkan13Features features{};
			features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			features.dynamicRendering = VK_TRUE;
			features.pNext = &buffer_device_address_features;

			VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{};
			descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
			descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
			descriptor_indexing_features.runtimeDescriptorArray = VK_TRUE;
			descriptor_indexing_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
			descriptor_indexing_features.pNext = &features;

			float queue_priority = 1.0f;
			VkDeviceQueueCreateInfo graphics_queue_create_info{};
			graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			graphics_queue_create_info.queueFamilyIndex = ctx->graphics_queue_index;
			graphics_queue_create_info.queueCount = 1;
			graphics_queue_create_info.pQueuePriorities = &queue_priority;

			VkDeviceCreateInfo device_create_info{};
			device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			device_create_info.pNext = &descriptor_indexing_features;
			device_create_info.queueCreateInfoCount = 1;
			device_create_info.pQueueCreateInfos = &graphics_queue_create_info;
			device_create_info.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
			device_create_info.ppEnabledExtensionNames = extensions;

			if (vkCreateDevice(ctx->gpu, &device_create_info, nullptr, &ctx->device) != VK_SUCCESS)
			{
				return false;
			}

			vkGetDeviceQueue(ctx->device, ctx->graphics_queue_index, 0, &ctx->queue);
		}

		// Create Allocator
		{
			VmaAllocatorCreateInfo vma_create_info{};
			vma_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
			vma_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
			vma_create_info.physicalDevice = ctx->gpu;
			vma_create_info.device = ctx->device;
			vma_create_info.instance = ctx->instance;

			if (vmaCreateAllocator(&vma_create_info, &ctx->allocator) != VK_SUCCESS)
			{
				return false;
			}
		}

		// Create Command Pool
		{
			VkCommandPoolCreateInfo command_pool_create_info{};
			command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			command_pool_create_info.queueFamilyIndex = ctx->graphics_queue_index;

			if (vkCreateCommandPool(ctx->device, &command_pool_create_info, nullptr, &ctx->command_pool) != VK_SUCCESS)
			{
				return false;
			}
		}

		return true;
	}

	bool CreateSwapchain(RendererContext* ctx)
	{

		// Create Swapchain
		{
			VkSurfaceFormatKHR preferred_formats[]{ { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } };

			uint32_t swapchain_format_count{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->gpu, ctx->surface, &swapchain_format_count, nullptr);
			VkSurfaceFormatKHR swapchain_formats[15]{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->gpu, ctx->surface, &swapchain_format_count, swapchain_formats);

			for (uint32_t i = 0; i < swapchain_format_count; ++i)
			{
				for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
				{
					if (preferred_formats[j].format == swapchain_formats[i].format &&
						preferred_formats[j].colorSpace == swapchain_formats[i].colorSpace)
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
				ctx->swapchain_format = swapchain_formats[0];
			}

			VkSurfaceCapabilitiesKHR capabilities{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->gpu, ctx->surface, &capabilities);

			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				ctx->swapchain_extent = capabilities.currentExtent;
			}
			else
			{
#ifdef OLIVIA_WIN32
				RECT rect;
				GetClientRect(static_cast<HWND>(ctx->window_handle), &rect);
				uint32_t width = rect.right - rect.left;
				uint32_t height = rect.bottom - rect.top;

				ctx->swapchain_extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				ctx->swapchain_extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
#endif // OLIVIA_WIN32
			}
			VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

			uint32_t swapchain_min_image_count = capabilities.minImageCount > 2u ? capabilities.minImageCount : 2u;

			if (ctx->vsync)
			{
				swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
			}
			else
			{
				uint32_t present_mode_count;
				vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->gpu, ctx->surface, &present_mode_count, nullptr);
				VkPresentModeKHR present_modes[8]{};
				vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->gpu, ctx->surface, &present_mode_count, present_modes);

				for (uint32_t i = 0; i < present_mode_count; ++i)
				{
					if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
					{
						swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
						swapchain_min_image_count = capabilities.minImageCount > 3u ? capabilities.minImageCount : 3u;
						break;
					}
					if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
					{
						swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
						swapchain_min_image_count = capabilities.minImageCount > 2u ? capabilities.minImageCount : 2u;
					}
				}
			}

			VkSwapchainCreateInfoKHR swapchain_create_info{};
			swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchain_create_info.surface = ctx->surface;
			swapchain_create_info.minImageCount = swapchain_min_image_count;
			swapchain_create_info.imageFormat = ctx->swapchain_format.format;
			swapchain_create_info.imageColorSpace = ctx->swapchain_format.colorSpace;
			swapchain_create_info.imageExtent = ctx->swapchain_extent;
			swapchain_create_info.imageArrayLayers = 1;
			swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchain_create_info.preTransform = capabilities.currentTransform;
			swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchain_create_info.presentMode = swapchain_present_mode;
			swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateSwapchainKHR(ctx->device, &swapchain_create_info, nullptr, &ctx->swapchain) != VK_SUCCESS)
			{
				return false;
			}
		}

		// Get Images
		{
			vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_size, nullptr);
			vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_size, ctx->swapchain_images);
		}

		// Create Views
		{
			for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
			{
				VkImageViewCreateInfo image_view_create_info{};
				image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				image_view_create_info.image = ctx->swapchain_images[i],
				image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D,
				image_view_create_info.format = ctx->swapchain_format.format,
				image_view_create_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
				image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				image_view_create_info.subresourceRange.baseMipLevel = 0;
				image_view_create_info.subresourceRange.levelCount = 1;
				image_view_create_info.subresourceRange.baseArrayLayer = 0;
				image_view_create_info.subresourceRange.layerCount = 1;

				if (vkCreateImageView(ctx->device, &image_view_create_info, nullptr, &ctx->swapchain_views[i]) != VK_SUCCESS)
				{
					return false;
				}
			}
		}

		return true;
	}

	bool CreateFrame(RendererContext* ctx)
	{
		// Allocate Command Buffers
		{
			// Allocate command buffers
			VkCommandBufferAllocateInfo command_buffer_alloc_info{};
			command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			command_buffer_alloc_info.commandPool = ctx->command_pool;
			command_buffer_alloc_info.commandBufferCount = MAX_CONCURRENT_FRAMES;
			command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			vkAllocateCommandBuffers(ctx->device, &command_buffer_alloc_info, ctx->command_buffers);
		}

		// Create Sync Objects
		{
			VkSemaphoreCreateInfo semaphore_create_info{};
			semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fence_create_info{};
			fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
			{
				if (vkCreateSemaphore(ctx->device, &semaphore_create_info, nullptr, &ctx->acquire_image[i]) != VK_SUCCESS)
				{
					return false;
				}

				if (vkCreateFence(ctx->device, &fence_create_info, nullptr, &ctx->queue_submit[i]) != VK_SUCCESS)
				{
					return false;
				}
			}

			for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
			{
				if (vkCreateSemaphore(ctx->device, &semaphore_create_info, nullptr, &ctx->present_image[i]) != VK_SUCCESS)
				{
					return false;
				}
			}
		}

		return true;
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
			.usage = memory_usage
		};

		Buffer b{};
		VK_CHECK(vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &b.buffer, &b.allocation, &b.info));

		return b;
	}

	void PremadeQuadMesh(Renderer* r)
	{
		// QUAD MESH DATA
				                          // Position           // Normal             // Color                    // UV           // Texture Index
		r->assets_manager.vertices[0] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, {0} };
		r->assets_manager.vertices[1] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, {0} };
		r->assets_manager.vertices[2] = { { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }, {0} };
		r->assets_manager.vertices[3] = { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, {0} };

		r->assets_manager.indices[0] = 0;
		r->assets_manager.indices[1] = 1;
		r->assets_manager.indices[2] = 2;
		
		r->assets_manager.indices[3] = 2;
		r->assets_manager.indices[4] = 3;
		r->assets_manager.indices[5] = 0;

		size_t vertices_size = 4 * sizeof(Vertex);
		size_t indices_size = 6 * sizeof(uint32_t);

		// HOLD MESH DATA
		uint32_t mesh = r->assets_manager.mesh_count;

		r->assets_manager.vertex_offset[mesh] = r->assets_manager.total_vertex_bytes;
		r->assets_manager.index_offset[mesh] = sizeof(r->assets_manager.vertices) + r->assets_manager.total_index_bytes;
		r->assets_manager.index_count[mesh] = 6;

		r->assets_manager.mesh_count++;

		memcpy((char*)r->assets_manager.mesh_buffer.info.pMappedData + r->assets_manager.vertex_offset[mesh], r->assets_manager.vertices, vertices_size);
		memcpy((char*)r->assets_manager.mesh_buffer.info.pMappedData + r->assets_manager.index_offset[mesh], r->assets_manager.indices, indices_size);

		r->assets_manager.total_vertex_bytes += vertices_size;
		r->assets_manager.total_index_bytes += indices_size;
	}

	bool CreateAssetManager(RendererContext* ctx, AssetManager* m)
	{
		// MESH BUFFER

		size_t mesh_buffer_size = sizeof(m->vertices) + sizeof(m->indices);

		m->mesh_buffer = CreateBuffer(
			ctx->allocator,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			mesh_buffer_size);

		return true;
	}

	Renderer* CreateRenderer(Allocator* allocator, Window* window)
	{
		Renderer* r = static_cast<Renderer*>(Allocate(allocator, sizeof(Renderer)));

		if (r)
		{
			g_renderer = r;

			// CONTEXT
			{
				r->ctx.window_handle = window->handle;
				
				// Create Device
				if (!CreateDevice(&r->ctx))
				{
					return nullptr;
				}

				// Create Swapchain
				if (!CreateSwapchain(&r->ctx))
				{
					return nullptr;
				}

				// Create Frame
				if (!CreateFrame(&r->ctx))
				{
					return nullptr;
				}
			}

			// MANAGERS
			{
				if (!CreateAssetManager(&r->ctx, &r->assets_manager))
				{
					return nullptr;
				}
			}

			// INIT
			{
				PremadeQuadMesh(r);
			}
		}

		return r;
	}

	Renderer* GetRenderer()
	{
		return nullptr;
	}

	void DestroyRenderer(Renderer* r)
	{
		assert(r && "Invalid pointer");
		
		AssetManager* assets_manager = &r->assets_manager;
		RendererContext* ctx = &r->ctx;

		vkDeviceWaitIdle(ctx->device);
		
		// DESTROY MANAGERS

		vmaDestroyBuffer(ctx->allocator, assets_manager->mesh_buffer.buffer, assets_manager->mesh_buffer.allocation);
		
		// DESTROY CONTEXT


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

	bool RecreateSwapchain(RendererContext* ctx)
	{
		// DESTROY FRAMES
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vkDestroySemaphore(ctx->device, ctx->acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
		{
			vkDestroySemaphore(ctx->device, ctx->present_image[i], nullptr);
			vkDestroyFence(ctx->device, ctx->queue_submit[i], nullptr);
		}

		// DESTROY OLD SWAPCHAIN
		vkDeviceWaitIdle(ctx->device);

		ctx->current_frame = 0;

		for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
		{
			vkDestroyImageView(ctx->device, ctx->swapchain_views[i], nullptr);
		}

		vkDestroySwapchainKHR(ctx->device, ctx->swapchain, nullptr);

		// RECREATE SYCHRONIZATION OBJECTS
		VkSemaphoreCreateInfo semaphore_create_info{};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_create_info{};
		fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			if (vkCreateSemaphore(ctx->device, &semaphore_create_info, nullptr, &ctx->acquire_image[i]) != VK_SUCCESS)
			{
				return false;
			}

			if (vkCreateFence(ctx->device, &fence_create_info, nullptr, &ctx->queue_submit[i]) != VK_SUCCESS)
			{
				return false;
			}
		}

		for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
		{
			if (vkCreateSemaphore(ctx->device, &semaphore_create_info, nullptr, &ctx->present_image[i]) != VK_SUCCESS)
			{
				return false;
			}
		}

		// RESET FRAME
		ctx->current_frame = 0;


		// RECREATE SWAPCHAIN
		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->gpu, ctx->surface, &capabilities);

		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			ctx->swapchain_extent = capabilities.currentExtent;
		}
		else
		{
#ifdef OLIVIA_WIN32
			RECT rect;
			GetClientRect(static_cast<HWND>(ctx->window_handle), &rect);
			uint32_t width = rect.right - rect.left;
			uint32_t height = rect.bottom - rect.top;

			ctx->swapchain_extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			ctx->swapchain_extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
#endif // OLIVIA_WIN32
		}
		VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

		uint32_t swapchain_min_image_count = capabilities.minImageCount > 2u ? capabilities.minImageCount : 2u;

		if (ctx->vsync)
		{
			swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
		}
		else
		{
			uint32_t present_mode_count;
			vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->gpu, ctx->surface, &present_mode_count, nullptr);
			VkPresentModeKHR present_modes[8]{};
			vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->gpu, ctx->surface, &present_mode_count, present_modes);

			for (uint32_t i = 0; i < present_mode_count; ++i)
			{
				if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
					swapchain_min_image_count = capabilities.minImageCount > 3u ? capabilities.minImageCount : 3u;
					break;
				}
				if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
					swapchain_min_image_count = capabilities.minImageCount > 2u ? capabilities.minImageCount : 2u;
				}
			}
		}

		VkSwapchainCreateInfoKHR swapchain_create_info{};
		swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_create_info.surface = ctx->surface;
		swapchain_create_info.minImageCount = swapchain_min_image_count;
		swapchain_create_info.imageFormat = ctx->swapchain_format.format;
		swapchain_create_info.imageColorSpace = ctx->swapchain_format.colorSpace;
		swapchain_create_info.imageExtent = ctx->swapchain_extent;
		swapchain_create_info.imageArrayLayers = 1;
		swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchain_create_info.preTransform = capabilities.currentTransform;
		swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_create_info.presentMode = swapchain_present_mode;
		swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateSwapchainKHR(ctx->device, &swapchain_create_info, nullptr, &ctx->swapchain) != VK_SUCCESS)
		{
			return false;
		}

		vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_size, nullptr);
		vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_size, ctx->swapchain_images);

		for (uint32_t i = 0; i < ctx->swapchain_size; ++i)
		{
			VkImageViewCreateInfo image_view_create_info{};
			image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				image_view_create_info.image = ctx->swapchain_images[i],
				image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D,
				image_view_create_info.format = ctx->swapchain_format.format,
				image_view_create_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_view_create_info.subresourceRange.baseMipLevel = 0;
			image_view_create_info.subresourceRange.levelCount = 1;
			image_view_create_info.subresourceRange.baseArrayLayer = 0;
			image_view_create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(ctx->device, &image_view_create_info, nullptr, &ctx->swapchain_views[i]) != VK_SUCCESS)
			{
				return false;
			}
		}

		printf("[OLIVIA] Swapchain recreated!\n");
		return true;
	}

	void Draw(Renderer* r)
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

		VkImageMemoryBarrier image_barrier_write{};
		image_barrier_write.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_barrier_write.srcAccessMask = VK_PIPELINE_STAGE_NONE;
		image_barrier_write.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		image_barrier_write.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_barrier_write.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_barrier_write.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_barrier_write.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_barrier_write.image = r->ctx.swapchain_images[r->ctx.image_index];
		image_barrier_write.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			r->ctx.command_buffers[r->ctx.current_frame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_write
		);

		VkRenderingAttachmentInfo color_attachment{};
		color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color_attachment.imageView = r->ctx.swapchain_views[r->ctx.image_index];
		color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };

		VkRect2D render_area{};
		render_area.offset = { 0 };
		render_area.extent = r->ctx.swapchain_extent;

		VkRenderingInfo render_info = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = render_area,
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment,
		};

		vkCmdBeginRendering(r->ctx.command_buffers[r->ctx.current_frame], &render_info);

		VkViewport viewport = {
			.width = (float)r->ctx.swapchain_extent.width,
			.height = (float)r->ctx.swapchain_extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D scissor = {
			.offset = {0, 0},
			.extent = r->ctx.swapchain_extent
		};

		vkCmdSetViewport(r->ctx.command_buffers[r->ctx.current_frame], 0, 1, &viewport);
		vkCmdSetScissor(r->ctx.command_buffers[r->ctx.current_frame], 0, 1, &scissor);

		// TODO: DRAW

		vkCmdEndRendering(r->ctx.command_buffers[r->ctx.current_frame]);

		VkImageMemoryBarrier image_barrier_present = {
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
			1, &image_barrier_present
		);

		vkEndCommandBuffer(r->ctx.command_buffers[r->ctx.current_frame]);

		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore wait_semaphores[] = { r->ctx.acquire_image[r->ctx.current_frame] };
		VkSemaphore signal_semaphores[] = { r->ctx.present_image[r->ctx.current_frame] };

		VkSubmitInfo submit_info = {
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

		VkSwapchainKHR swapchains[] = { r->ctx.swapchain };

		VkPresentInfoKHR present_info = {
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

	VkShaderModule CreateShaderModule(VkDevice device, const char* shader)
	{
		size_t code_size;
		uint8_t* shader_code = static_cast<uint8_t*>(ReadFileR(shader, &code_size));

		VkShaderModuleCreateInfo shader_module_ci
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = code_size,
			.pCode = (const uint32_t*)shader_code
		};

		VkShaderModule shader_module{ VK_NULL_HANDLE };
		VK_CHECK(vkCreateShaderModule(device, &shader_module_ci, nullptr, &shader_module));

		FreeFileR(shader_code);

		return shader_module;
	}

} // Olivia