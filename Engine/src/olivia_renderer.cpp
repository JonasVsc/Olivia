#include "olivia/renderer/olivia_renderer_internal.h"

#include <SDL3/SDL_vulkan.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace olivia
{
	renderer init_renderer(lifetime_allocator allocator, SDL_Window* window)
	{
		renderer result_renderer = (renderer)allocate(allocator, sizeof(renderer_t));

		if (result_renderer)
		{
			result_renderer->window = window;

			init_vulkan_core(result_renderer);
		}

		return result_renderer;
	}

	void begin_frame(renderer renderer)
	{
		vulkan_core_t& core = renderer->core;

		vkWaitForFences(core.device, 1, &core.queue_submit[core.current_frame], VK_TRUE, UINT64_MAX);
		vkResetFences(core.device, 1, &core.queue_submit[core.current_frame]);

		VkResult acquire_result = vkAcquireNextImageKHR(core.device, core.swapchain, UINT64_MAX, core.acquire_image[core.current_frame], VK_NULL_HANDLE, &core.image_index);
		if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR)
		{
			LOG_INFO(TAG_RENDERER, "Recreating swapchain");
			recreate_swapchain(renderer);
			return;
		}
		else if (acquire_result != VK_SUCCESS)
		{
			LOG_ERROR(TAG_RENDERER, "Failed to acquire next image");
			abort();
		}

		vkResetCommandBuffer(core.command_buffers[core.current_frame], 0);
		VkCommandBufferBeginInfo cmd_begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(core.command_buffers[core.current_frame], &cmd_begin);

		VkImageMemoryBarrier image_barrier_write
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_PIPELINE_STAGE_NONE,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = core.swapchain_images[core.image_index],
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};

		vkCmdPipelineBarrier(
			core.command_buffers[core.current_frame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_write);

		VkRenderingAttachmentInfo color_attachment
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = core.swapchain_views[core.image_index],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {{0.0f, 0.0f, 0.0f, 0.0f}}
		};

		VkRect2D render_area
		{
			.offset = { 0 },
			.extent = core.swapchain_extent
		};

		VkRenderingInfo render_info
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = render_area,
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment,
		};

		vkCmdBeginRendering(core.command_buffers[core.current_frame], &render_info);

		VkViewport viewport
		{
			.width = (float)core.swapchain_extent.width,
			.height = (float)core.swapchain_extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D scissor
		{
			.offset = {0, 0},
			.extent = core.swapchain_extent
		};

		vkCmdSetViewport(core.command_buffers[core.current_frame], 0, 1, &viewport);
		vkCmdSetScissor(core.command_buffers[core.current_frame], 0, 1, &scissor);
	}

	void end_frame(renderer renderer)
	{
		vulkan_core_t& core = renderer->core;

		vkCmdEndRendering(core.command_buffers[core.current_frame]);

		VkImageMemoryBarrier image_barrier_present
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = core.swapchain_images[core.image_index],
			.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		};

		vkCmdPipelineBarrier(
			core.command_buffers[core.current_frame],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_present);

		vkEndCommandBuffer(core.command_buffers[core.current_frame]);

		VkPipelineStageFlags wait_stages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore wait_semaphores[]{ core.acquire_image[core.current_frame] };
		VkSemaphore signal_semaphores[]{ core.present_image[core.current_frame] };

		VkSubmitInfo submit_info
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = wait_semaphores,
			.pWaitDstStageMask = wait_stages,
			.commandBufferCount = 1,
			.pCommandBuffers = &core.command_buffers[core.current_frame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signal_semaphores
		};

		vkQueueSubmit(core.queue, 1, &submit_info, core.queue_submit[core.current_frame]);

		VkSwapchainKHR swapchains[]{ core.swapchain };

		VkPresentInfoKHR present_info
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signal_semaphores,
			.swapchainCount = 1,
			.pSwapchains = swapchains,
			.pImageIndices = &core.image_index
		};

		VkResult present_result = vkQueuePresentKHR(core.queue, &present_info);
		if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
		{
			LOG_INFO(TAG_RENDERER, "Recreating swapchain");
			recreate_swapchain(renderer);
			return;
		}
		else if (present_result != VK_SUCCESS)
		{
			LOG_ERROR(TAG_RENDERER, "Failed to present image");
			return;
		}

		core.current_frame = (core.current_frame + 1) % MAX_FRAMES;
	}

	void init_vulkan_core(renderer renderer)
	{
		vulkan_core_t& core = renderer->core;

		// create instance
		{
			VkApplicationInfo application_info
			{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Olivia",
				.pEngineName = "Olivia",
				.apiVersion = VK_API_VERSION_1_3
			};

			uint32_t sdl_extension_count;
			char const* const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);

			VkInstanceCreateInfo instance_info
			{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pApplicationInfo = &application_info,
				.enabledExtensionCount = sdl_extension_count,
				.ppEnabledExtensionNames = sdl_extensions
			};

#ifdef OLIVIA_DEBUG
			const char* layers[]
			{
				"VK_LAYER_KHRONOS_validation"
			};

			instance_info.enabledLayerCount = ARRAY_SIZE(layers);
			instance_info.ppEnabledLayerNames = layers;
#endif // OLIVIA_DEBUG

			VK_CHECK(vkCreateInstance(&instance_info, nullptr, &core.instance));
		}

		// create surface
		{
			bool res = SDL_Vulkan_CreateSurface(renderer->window, core.instance, nullptr, &core.surface);
			if (!res)
			{
				LOG_ERROR(TAG_RENDERER, "%s", SDL_GetError());
				abort();
			}
		}

		// create device
		{
			uint32_t gpu_count{};
			vkEnumeratePhysicalDevices(core.instance, &gpu_count, nullptr);

			if (gpu_count <= 0)
			{

				printf("No suitable gpu found\n");
				abort();
			}

			VkPhysicalDevice gpus[10]{};
			vkEnumeratePhysicalDevices(core.instance, &gpu_count, gpus);

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

				core.graphics_queue_index = UINT32_MAX;

				for (uint32_t i = 0; i < queue_family_count; ++i)
				{
					VkBool32 support_presentation{ VK_FALSE };
					vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], i, core.surface, &support_presentation);

					if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && support_presentation)
					{
						core.graphics_queue_index = i;
						core.gpu = gpus[i];
						break;
					}
				}
			}

			if (core.graphics_queue_index == UINT32_MAX)
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
					.queueFamilyIndex = core.graphics_queue_index,
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

			VK_CHECK(vkCreateDevice(core.gpu, &device_info, nullptr, &core.device));

			vkGetDeviceQueue(core.device, core.graphics_queue_index, 0, &core.queue);
		}

		// create command_pool
		{
			VkCommandPoolCreateInfo command_pool_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = core.graphics_queue_index
			};

			VK_CHECK(vkCreateCommandPool(core.device, &command_pool_info, nullptr, &core.command_pool));
		}

		// create swapchain
		{
			VkSurfaceFormatKHR preferred_formats[]
			{
				{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
			};

			uint32_t format_count{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(core.gpu, core.surface, &format_count, nullptr);
			VkSurfaceFormatKHR formats[15]{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(core.gpu, core.surface, &format_count, formats);

			for (uint32_t i = 0; i < format_count; ++i)
			{
				for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
				{
					if (preferred_formats[j].format == formats[i].format &&
						preferred_formats[j].colorSpace == formats[i].colorSpace)
					{
						core.swapchain_format = preferred_formats[j];
						break;
					}
				}

				if (core.swapchain_format.format != VK_FORMAT_UNDEFINED)
				{
					break;
				}
			}

			if (core.swapchain_format.format == VK_FORMAT_UNDEFINED)
			{
				core.swapchain_format = formats[0];
			}

			VkSurfaceCapabilitiesKHR capabilities{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(core.gpu, core.surface, &capabilities);

			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				core.swapchain_extent = capabilities.currentExtent;
			}
			else
			{
				int width, height;
				SDL_GetWindowSizeInPixels(renderer->window, &width, &height);
				core.swapchain_extent = { (uint32_t)width, (uint32_t)height };
				SDL_clamp(core.swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				SDL_clamp(core.swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			}

			uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

			VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

			VkSwapchainCreateInfoKHR swapchain_info
			{
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = core.surface,
				.minImageCount = min_image_count,
				.imageFormat = core.swapchain_format.format,
				.imageColorSpace = core.swapchain_format.colorSpace,
				.imageExtent = core.swapchain_extent,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.preTransform = capabilities.currentTransform,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = presentation_mode,
				.oldSwapchain = VK_NULL_HANDLE,
			};

			VK_CHECK(vkCreateSwapchainKHR(core.device, &swapchain_info, nullptr, &core.swapchain));
		}

		// create swapchain images
		{
			vkGetSwapchainImagesKHR(core.device, core.swapchain, &core.swapchain_size, nullptr);
			vkGetSwapchainImagesKHR(core.device, core.swapchain, &core.swapchain_size, core.swapchain_images);
		}

		// create swapchain views
		{
			for (uint32_t i = 0; i < core.swapchain_size; ++i)
			{
				VkImageViewCreateInfo image_view_info
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = core.swapchain_images[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = core.swapchain_format.format,
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

				VK_CHECK(vkCreateImageView(core.device, &image_view_info, nullptr, &core.swapchain_views[i]));
			}
		}

		// allocate command buffers
		{
			VkCommandBufferAllocateInfo command_buffer_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = core.command_pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = MAX_FRAMES
			};

			VK_CHECK(vkAllocateCommandBuffers(core.device, &command_buffer_info, core.command_buffers));
		}

		// create semaphores & fences
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


			for (uint32_t i = 0; i < MAX_FRAMES; ++i)
			{
				VK_CHECK(vkCreateSemaphore(core.device, &semaphore_info, nullptr, &core.acquire_image[i]));
				VK_CHECK(vkCreateFence(core.device, &fence_info, nullptr, &core.queue_submit[i]));
			}

			for (uint32_t i = 0; i < core.swapchain_size; ++i)
			{
				VK_CHECK(vkCreateSemaphore(core.device, &semaphore_info, nullptr, &core.present_image[i]));
			}
		}
	}

	void recreate_swapchain(renderer renderer)
	{
		vulkan_core_t& core = renderer->core;

		// Create VkSwapchainKHR
		{
			VkSurfaceFormatKHR preferred_formats[]
			{
				{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
			};

			uint32_t format_count{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(core.gpu, core.surface, &format_count, nullptr);
			VkSurfaceFormatKHR formats[15]{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(core.gpu, core.surface, &format_count, formats);

			for (uint32_t i = 0; i < format_count; ++i)
			{
				for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
				{
					if (preferred_formats[j].format == formats[i].format &&
						preferred_formats[j].colorSpace == formats[i].colorSpace)
					{
						core.swapchain_format = preferred_formats[j];
						break;
					}
				}

				if (core.swapchain_format.format != VK_FORMAT_UNDEFINED)
				{
					break;
				}
			}

			if (core.swapchain_format.format == VK_FORMAT_UNDEFINED)
			{
				core.swapchain_format = formats[0];
			}

			VkSurfaceCapabilitiesKHR capabilities{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(core.gpu, core.surface, &capabilities);

			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				core.swapchain_extent = capabilities.currentExtent;
			}
			else
			{
				int width, height;
				SDL_GetWindowSizeInPixels(renderer->window, &width, &height);
				core.swapchain_extent = { (uint32_t)width, (uint32_t)height };
				SDL_clamp(core.swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				SDL_clamp(core.swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			}

			uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

			VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

			VkSwapchainCreateInfoKHR swapchain_info
			{
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = core.surface,
				.minImageCount = min_image_count,
				.imageFormat = core.swapchain_format.format,
				.imageColorSpace = core.swapchain_format.colorSpace,
				.imageExtent = core.swapchain_extent,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.preTransform = capabilities.currentTransform,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = presentation_mode,
				.oldSwapchain = VK_NULL_HANDLE,
			};

			VK_CHECK(vkCreateSwapchainKHR(core.device, &swapchain_info, nullptr, &core.swapchain));
		}

		// Swapchain VkImages
		{
			vkGetSwapchainImagesKHR(core.device, core.swapchain, &core.swapchain_size, nullptr);
			vkGetSwapchainImagesKHR(core.device, core.swapchain, &core.swapchain_size, core.swapchain_images);
		}

		// Swapchain VkImageViews
		{
			for (uint32_t i = 0; i < core.swapchain_size; ++i)
			{
				VkImageViewCreateInfo image_view_info
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = core.swapchain_images[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = core.swapchain_format.format,
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

				VK_CHECK(vkCreateImageView(core.device, &image_view_info, nullptr, &core.swapchain_views[i]));
			}
		}
	}

}