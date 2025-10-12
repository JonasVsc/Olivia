#include "Vulkan_Renderer.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#ifdef OLIVIA_WIN32
	#include "Platform/Win32/Platform.h"
	#include "vulkan/vulkan_win32.h"
#endif

namespace Olivia
{

	Renderer* CreateRenderer(Allocator* allocator, Window* window)
	{
		Renderer* r = static_cast<Renderer*>(Allocate(allocator, sizeof(Renderer)));

		if (r)
		{
			r->window_handle = window->handle;

			// Create Device
			if (!CreateDevice(r))
			{
				return nullptr;
			}

			// Create Swapchain
			if (!CreateSwapchain(r))
			{
				return nullptr;
			}

			// Create Frame
			if (!CreateFrame(r))
			{
				return nullptr;
			}
		}

		return r;
	}

	bool CreateDevice(Renderer* r)
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

			if (vkCreateInstance(&instance_create_info, nullptr, &r->instance) != VK_SUCCESS)
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
			win32_surface_create_info.hwnd = static_cast<HWND>(r->window_handle);

			if (vkCreateWin32SurfaceKHR(r->instance, &win32_surface_create_info, nullptr, &r->surface) != VK_SUCCESS)
			{
				return false;
			}

#endif // OLIVIA_WIN32
		}

		// Select GPU 
		{
			uint32_t gpu_count{};
			vkEnumeratePhysicalDevices(r->instance, &gpu_count, nullptr);

			if (gpu_count <= 0)
				abort();

			VkPhysicalDevice gpus[10] = { 0 };
			vkEnumeratePhysicalDevices(r->instance, &gpu_count, gpus);

			for (uint32_t i = 0; i < gpu_count; ++i)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpus[i], &properties);

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					r->gpu = gpus[i];
					break;
				}
			}
			if (r->gpu == VK_NULL_HANDLE)
			{
				r->gpu = gpus[0];
			}
		}

		// Create device
		{
			uint32_t gpu_count{};
			vkEnumeratePhysicalDevices(r->instance, &gpu_count, nullptr);

			if (gpu_count < 1)
				abort();

			VkPhysicalDevice gpus[10] = { 0 };
			vkEnumeratePhysicalDevices(r->instance, &gpu_count, gpus);

			for (uint32_t i = 0; i < gpu_count; ++i)
			{
				VkPhysicalDeviceProperties device_properties;
				vkGetPhysicalDeviceProperties(gpus[i], &device_properties);

				if (device_properties.apiVersion < VK_API_VERSION_1_3)
				{
					continue;
				}

				uint32_t queue_family_count = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(r->gpu, &queue_family_count, nullptr);

				VkQueueFamilyProperties queue_family_properties[20] = { 0 };
				vkGetPhysicalDeviceQueueFamilyProperties(r->gpu, &queue_family_count, queue_family_properties);

				for (uint32_t i = 0; i < queue_family_count; ++i)
				{
					VkBool32 supports_present = VK_FALSE;
					vkGetPhysicalDeviceSurfaceSupportKHR(r->gpu, i, r->surface, &supports_present);

					if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present)
					{
						r->graphics_queue_index = i;
						break;
					}
				}

				if (r->graphics_queue_index >= 0)
				{
					r->gpu = gpus[i];
					break;
				}
			}

			if (r->graphics_queue_index < 0)
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
			graphics_queue_create_info.queueFamilyIndex = r->graphics_queue_index;
			graphics_queue_create_info.queueCount = 1;
			graphics_queue_create_info.pQueuePriorities = &queue_priority;

			VkDeviceCreateInfo device_create_info{};
			device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			device_create_info.pNext = &descriptor_indexing_features;
			device_create_info.queueCreateInfoCount = 1;
			device_create_info.pQueueCreateInfos = &graphics_queue_create_info;
			device_create_info.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
			device_create_info.ppEnabledExtensionNames = extensions;

			if (vkCreateDevice(r->gpu, &device_create_info, nullptr, &r->device) != VK_SUCCESS)
			{
				return false;
			}

			vkGetDeviceQueue(r->device, r->graphics_queue_index, 0, &r->queue);
		}

		// Create Allocator
		{
			VmaAllocatorCreateInfo vma_create_info{};
			vma_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
			vma_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
			vma_create_info.physicalDevice = r->gpu;
			vma_create_info.device = r->device;
			vma_create_info.instance = r->instance;

			if (vmaCreateAllocator(&vma_create_info, &r->allocator) != VK_SUCCESS)
			{
				return false;
			}
		}

		// Create Command Pool
		{
			VkCommandPoolCreateInfo command_pool_create_info{};
			command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			command_pool_create_info.queueFamilyIndex = r->graphics_queue_index;

			if (vkCreateCommandPool(r->device, &command_pool_create_info, nullptr, &r->command_pool) != VK_SUCCESS)
			{
				return false;
			}
		}

		return true;

	}

	bool Olivia::CreateSwapchain(Renderer* r)
	{

		// Create Swapchain
		{
			VkSurfaceFormatKHR preferred_formats[]{ { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } };

			uint32_t swapchain_format_count{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(r->gpu, r->surface, &swapchain_format_count, nullptr);
			VkSurfaceFormatKHR swapchain_formats[15]{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(r->gpu, r->surface, &swapchain_format_count, swapchain_formats);

			for (uint32_t i = 0; i < swapchain_format_count; ++i)
			{
				for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
				{
					if (preferred_formats[j].format == swapchain_formats[i].format &&
						preferred_formats[j].colorSpace == swapchain_formats[i].colorSpace)
					{
						r->swapchain_format = preferred_formats[j];
						break;
					}
				}

				if (r->swapchain_format.format != VK_FORMAT_UNDEFINED)
				{
					break;
				}
			}

			if (r->swapchain_format.format == VK_FORMAT_UNDEFINED)
			{
				r->swapchain_format = swapchain_formats[0];
			}

			VkSurfaceCapabilitiesKHR capabilities{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r->gpu, r->surface, &capabilities);

			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				r->swapchain_extent = capabilities.currentExtent;
			}
			else
			{
#ifdef OLIVIA_WIN32
				RECT rect;
				GetClientRect(static_cast<HWND>(r->window_handle), &rect);
				uint32_t width = rect.right - rect.left;
				uint32_t height = rect.bottom - rect.top;

				r->swapchain_extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				r->swapchain_extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
#endif // OLIVIA_WIN32
			}
			VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

			uint32_t swapchain_min_image_count = capabilities.minImageCount > 2u ? capabilities.minImageCount : 2u;

			if (r->vsync)
			{
				swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
			}
			else
			{
				uint32_t present_mode_count;
				vkGetPhysicalDeviceSurfacePresentModesKHR(r->gpu, r->surface, &present_mode_count, nullptr);
				VkPresentModeKHR present_modes[8]{};
				vkGetPhysicalDeviceSurfacePresentModesKHR(r->gpu, r->surface, &present_mode_count, present_modes);

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
			swapchain_create_info.surface = r->surface;
			swapchain_create_info.minImageCount = swapchain_min_image_count;
			swapchain_create_info.imageFormat = r->swapchain_format.format;
			swapchain_create_info.imageColorSpace = r->swapchain_format.colorSpace;
			swapchain_create_info.imageExtent = r->swapchain_extent;
			swapchain_create_info.imageArrayLayers = 1;
			swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchain_create_info.preTransform = capabilities.currentTransform;
			swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchain_create_info.presentMode = swapchain_present_mode;
			swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateSwapchainKHR(r->device, &swapchain_create_info, nullptr, &r->swapchain) != VK_SUCCESS)
			{
				return false;
			}
		}

		// Get Images
		{
			vkGetSwapchainImagesKHR(r->device, r->swapchain, &r->swapchain_size, nullptr);
			vkGetSwapchainImagesKHR(r->device, r->swapchain, &r->swapchain_size, r->swapchain_images);
		}

		// Create Views
		{
			for (uint32_t i = 0; i < r->swapchain_size; ++i)
			{
				VkImageViewCreateInfo image_view_create_info{};
				image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				image_view_create_info.image = r->swapchain_images[i],
				image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D,
				image_view_create_info.format = r->swapchain_format.format,
				image_view_create_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
				image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				image_view_create_info.subresourceRange.baseMipLevel = 0;
				image_view_create_info.subresourceRange.levelCount = 1;
				image_view_create_info.subresourceRange.baseArrayLayer = 0;
				image_view_create_info.subresourceRange.layerCount = 1;

				if (vkCreateImageView(r->device, &image_view_create_info, nullptr, &r->swapchain_views[i]) != VK_SUCCESS)
				{
					return false;
				}
			}
		}

		return true;
	}

	bool CreateFrame(Renderer* r)
	{
		// Allocate Command Buffers
		{
			// Allocate command buffers
			VkCommandBufferAllocateInfo command_buffer_alloc_info{};
			command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			command_buffer_alloc_info.commandPool = r->command_pool;
			command_buffer_alloc_info.commandBufferCount = MAX_CONCURRENT_FRAMES;
			command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			vkAllocateCommandBuffers(r->device, &command_buffer_alloc_info, r->command_buffers);
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
				if (vkCreateSemaphore(r->device, &semaphore_create_info, nullptr, &r->acquire_image[i]) != VK_SUCCESS)
				{
					return false;
				}

				if (vkCreateFence(r->device, &fence_create_info, nullptr, &r->queue_submit[i]) != VK_SUCCESS)
				{
					return false;
				}
			}

			for (uint32_t i = 0; i < r->swapchain_size; ++i)
			{
				if (vkCreateSemaphore(r->device, &semaphore_create_info, nullptr, &r->present_image[i]) != VK_SUCCESS)
				{
					return false;
				}
			}
		}

		return true;
	}

	void Draw(Renderer* r)
	{
		vkWaitForFences(r->device, 1, &r->queue_submit[r->current_frame], VK_TRUE, UINT64_MAX);
		vkResetFences(r->device, 1, &r->queue_submit[r->current_frame]);

		VkResult acquire_result = vkAcquireNextImageKHR(r->device, r->swapchain, UINT64_MAX, r->acquire_image[r->current_frame], VK_NULL_HANDLE, &r->image_index);
		if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR)
		{
			printf("[OLIVIA] Recreating swapchain\n");
			RecreateSwapchain(r);
			return;
		}
		else if (acquire_result != VK_SUCCESS)
		{
			printf("[OLIVIA] Failed to acquire next image!\n");
			abort();
		}

		vkResetCommandBuffer(r->command_buffers[r->current_frame], 0);
		VkCommandBufferBeginInfo cmd_begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(r->command_buffers[r->current_frame], &cmd_begin);

		VkImageMemoryBarrier image_barrier_write{};
		image_barrier_write.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_barrier_write.srcAccessMask = VK_PIPELINE_STAGE_NONE;
		image_barrier_write.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		image_barrier_write.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_barrier_write.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_barrier_write.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_barrier_write.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_barrier_write.image = r->swapchain_images[r->image_index];
		image_barrier_write.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			r->command_buffers[r->current_frame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_write
		);

		VkRenderingAttachmentInfo color_attachment{};
		color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color_attachment.imageView = r->swapchain_views[r->image_index];
		color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };

		VkRect2D render_area{};
		render_area.offset = { 0 };
		render_area.extent = r->swapchain_extent;

		VkRenderingInfo render_info = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = render_area,
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment,
		};

		vkCmdBeginRendering(r->command_buffers[r->current_frame], &render_info);

		VkViewport viewport = {
			.width = (float)r->swapchain_extent.width,
			.height = (float)r->swapchain_extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D scissor = {
			.offset = {0, 0},
			.extent = r->swapchain_extent
		};

		vkCmdSetViewport(r->command_buffers[r->current_frame], 0, 1, &viewport);
		vkCmdSetScissor(r->command_buffers[r->current_frame], 0, 1, &scissor);

		// TODO: DRAW

		vkCmdEndRendering(r->command_buffers[r->current_frame]);

		VkImageMemoryBarrier image_barrier_present = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = r->swapchain_images[r->image_index],
			.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		};

		vkCmdPipelineBarrier(
			r->command_buffers[r->current_frame],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_present
		);

		vkEndCommandBuffer(r->command_buffers[r->current_frame]);

		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore wait_semaphores[] = { r->acquire_image[r->current_frame] };
		VkSemaphore signal_semaphores[] = { r->present_image[r->current_frame] };

		VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = wait_semaphores,
			.pWaitDstStageMask = wait_stages,
			.commandBufferCount = 1,
			.pCommandBuffers = &r->command_buffers[r->current_frame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signal_semaphores
		};

		vkQueueSubmit(r->queue, 1, &submit_info, r->queue_submit[r->current_frame]);

		VkSwapchainKHR swapchains[] = { r->swapchain };

		VkPresentInfoKHR present_info = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signal_semaphores,
			.swapchainCount = 1,
			.pSwapchains = swapchains,
			.pImageIndices = &r->image_index
		};

		VkResult present_result = vkQueuePresentKHR(r->queue, &present_info);
		if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
		{
			printf("[OLIVIA] Recreating swapchain\n");
			RecreateSwapchain(r);
			return;
		}
		else if (present_result != VK_SUCCESS)
		{
			printf("[OLIVIA] Failed to present image!\n");
			return;
		}

		r->current_frame = (r->current_frame + 1) % MAX_CONCURRENT_FRAMES;
	}

	bool RecreateSwapchain(Renderer* r)
	{
		// DESTROY FRAMES
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vkDestroySemaphore(r->device, r->acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < r->swapchain_size; ++i)
		{
			vkDestroySemaphore(r->device, r->present_image[i], nullptr);
			vkDestroyFence(r->device, r->queue_submit[i], nullptr);
		}

		// DESTROY OLD SWAPCHAIN
		vkDeviceWaitIdle(r->device);

		r->current_frame = 0;

		for (uint32_t i = 0; i < r->swapchain_size; ++i)
		{
			vkDestroyImageView(r->device, r->swapchain_views[i], nullptr);
		}

		vkDestroySwapchainKHR(r->device, r->swapchain, nullptr);

		// RECREATE SYCHRONIZATION OBJECTS
		VkSemaphoreCreateInfo semaphore_create_info{};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_create_info{};
		fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			if (vkCreateSemaphore(r->device, &semaphore_create_info, nullptr, &r->acquire_image[i]) != VK_SUCCESS)
			{
				return false;
			}

			if (vkCreateFence(r->device, &fence_create_info, nullptr, &r->queue_submit[i]) != VK_SUCCESS)
			{
				return false;
			}
		}

		for (uint32_t i = 0; i < r->swapchain_size; ++i)
		{
			if (vkCreateSemaphore(r->device, &semaphore_create_info, nullptr, &r->present_image[i]) != VK_SUCCESS)
			{
				return false;
			}
		}

		// RESET FRAME
		r->current_frame = 0;


		// RECREATE SWAPCHAIN
		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r->gpu, r->surface, &capabilities);

		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			r->swapchain_extent = capabilities.currentExtent;
		}
		else
		{
#ifdef OLIVIA_WIN32
			RECT rect;
			GetClientRect(static_cast<HWND>(r->window_handle), &rect);
			uint32_t width = rect.right - rect.left;
			uint32_t height = rect.bottom - rect.top;

			r->swapchain_extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			r->swapchain_extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
#endif // OLIVIA_WIN32
		}
		VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

		uint32_t swapchain_min_image_count = capabilities.minImageCount > 2u ? capabilities.minImageCount : 2u;

		if (r->vsync)
		{
			swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
		}
		else
		{
			uint32_t present_mode_count;
			vkGetPhysicalDeviceSurfacePresentModesKHR(r->gpu, r->surface, &present_mode_count, nullptr);
			VkPresentModeKHR present_modes[8]{};
			vkGetPhysicalDeviceSurfacePresentModesKHR(r->gpu, r->surface, &present_mode_count, present_modes);

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
		swapchain_create_info.surface = r->surface;
		swapchain_create_info.minImageCount = swapchain_min_image_count;
		swapchain_create_info.imageFormat = r->swapchain_format.format;
		swapchain_create_info.imageColorSpace = r->swapchain_format.colorSpace;
		swapchain_create_info.imageExtent = r->swapchain_extent;
		swapchain_create_info.imageArrayLayers = 1;
		swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchain_create_info.preTransform = capabilities.currentTransform;
		swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_create_info.presentMode = swapchain_present_mode;
		swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateSwapchainKHR(r->device, &swapchain_create_info, nullptr, &r->swapchain) != VK_SUCCESS)
		{
			return false;
		}

		vkGetSwapchainImagesKHR(r->device, r->swapchain, &r->swapchain_size, nullptr);
		vkGetSwapchainImagesKHR(r->device, r->swapchain, &r->swapchain_size, r->swapchain_images);

		for (uint32_t i = 0; i < r->swapchain_size; ++i)
		{
			VkImageViewCreateInfo image_view_create_info{};
			image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			image_view_create_info.image = r->swapchain_images[i],
			image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D,
			image_view_create_info.format = r->swapchain_format.format,
			image_view_create_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_view_create_info.subresourceRange.baseMipLevel = 0;
			image_view_create_info.subresourceRange.levelCount = 1;
			image_view_create_info.subresourceRange.baseArrayLayer = 0;
			image_view_create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(r->device, &image_view_create_info, nullptr, &r->swapchain_views[i]) != VK_SUCCESS)
			{
				return false;
			}
		}

		printf("[OLIVIA] Swapchain recreated!\n");
		return true;
	}

	void DestroyRenderer(Renderer* r)
	{
		assert(r && "Invalid pointer");

		vkDeviceWaitIdle(r->device);

		// TracyVkDestroy(tctx);

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vkDestroySemaphore(r->device, r->acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < r->swapchain_size; ++i)
		{
			vkDestroyImageView(r->device, r->swapchain_views[i], nullptr);
			vkDestroySemaphore(r->device, r->present_image[i], nullptr);
			vkDestroyFence(r->device, r->queue_submit[i], nullptr);
		}

		vkDestroyCommandPool(r->device, r->command_pool, nullptr);
		vkDestroySwapchainKHR(r->device, r->swapchain, nullptr);
		vmaDestroyAllocator(r->allocator);

		vkDestroyDevice(r->device, nullptr);
		vkDestroySurfaceKHR(r->instance, r->surface, nullptr);
		vkDestroyInstance(r->instance, nullptr);
	}

} // Olivia