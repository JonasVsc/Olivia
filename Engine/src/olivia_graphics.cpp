#include "olivia/olivia_graphics.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace olivia
{
	static vulkan_core_t vulkan_core{};

	void init_vulkan_core(SDL_Window* window)
	{
		if (!window)
			return;

		vulkan_core.window = window;

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

			VK_CHECK(vkCreateInstance(&instance_info, nullptr, &vulkan_core.instance));
		}

		// create surface
		{
			bool res = SDL_Vulkan_CreateSurface(vulkan_core.window, vulkan_core.instance, nullptr, &vulkan_core.surface);
			if (!res)
			{
				printf("SDL_Error: %s\n", SDL_GetError());
				abort();
			}
		}

		// create device
		{
			uint32_t gpu_count{};
			vkEnumeratePhysicalDevices(vulkan_core.instance, &gpu_count, nullptr);

			if (gpu_count <= 0)
			{

				printf("No suitable gpu found\n");
				abort();
			}

			VkPhysicalDevice gpus[10]{};
			vkEnumeratePhysicalDevices(vulkan_core.instance, &gpu_count, gpus);

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

				vulkan_core.graphics_queue_index = UINT32_MAX;

				for (uint32_t i = 0; i < queue_family_count; ++i)
				{
					VkBool32 support_presentation{ VK_FALSE };
					vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], i, vulkan_core.surface, &support_presentation);

					if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && support_presentation)
					{
						vulkan_core.graphics_queue_index = i;
						vulkan_core.gpu = gpus[i];
						break;
					}
				}
			}

			if (vulkan_core.graphics_queue_index == UINT32_MAX)
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
					.queueFamilyIndex = vulkan_core.graphics_queue_index,
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

			VK_CHECK(vkCreateDevice(vulkan_core.gpu, &device_info, nullptr, &vulkan_core.device));

			vkGetDeviceQueue(vulkan_core.device, vulkan_core.graphics_queue_index, 0, &vulkan_core.queue);
		}

		// create allocator
		{
			VmaAllocatorCreateInfo vma_info
			{
				.physicalDevice = vulkan_core.gpu,
				.device = vulkan_core.device,
				.instance = vulkan_core.instance,
				.vulkanApiVersion = VK_API_VERSION_1_3
			};

			VK_CHECK(vmaCreateAllocator(&vma_info, &vulkan_core.allocator));
		}

		// create command_pool
		{
			VkCommandPoolCreateInfo command_pool_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = vulkan_core.graphics_queue_index
			};

			VK_CHECK(vkCreateCommandPool(vulkan_core.device, &command_pool_info, nullptr, &vulkan_core.command_pool));
		}

		// create swapchain
		{
			VkSurfaceFormatKHR preferred_formats[]
			{
				{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
			};

			uint32_t format_count{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_core.gpu, vulkan_core.surface, &format_count, nullptr);
			VkSurfaceFormatKHR formats[15]{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_core.gpu, vulkan_core.surface, &format_count, formats);

			for (uint32_t i = 0; i < format_count; ++i)
			{
				for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
				{
					if (preferred_formats[j].format == formats[i].format &&
						preferred_formats[j].colorSpace == formats[i].colorSpace)
					{
						vulkan_core.swapchain_format = preferred_formats[j];
						break;
					}
				}

				if (vulkan_core.swapchain_format.format != VK_FORMAT_UNDEFINED)
				{
					break;
				}
			}

			if (vulkan_core.swapchain_format.format == VK_FORMAT_UNDEFINED)
			{
				vulkan_core.swapchain_format = formats[0];
			}

			VkSurfaceCapabilitiesKHR capabilities{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_core.gpu, vulkan_core.surface, &capabilities);

			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				vulkan_core.swapchain_extent = capabilities.currentExtent;
			}
			else
			{
				int width, height;
				SDL_GetWindowSizeInPixels(vulkan_core.window, &width, &height);
				vulkan_core.swapchain_extent = { (uint32_t)width, (uint32_t)height };
				SDL_clamp(vulkan_core.swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				SDL_clamp(vulkan_core.swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			}

			uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

			VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

			VkSwapchainCreateInfoKHR swapchain_info
			{
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = vulkan_core.surface,
				.minImageCount = min_image_count,
				.imageFormat = vulkan_core.swapchain_format.format,
				.imageColorSpace = vulkan_core.swapchain_format.colorSpace,
				.imageExtent = vulkan_core.swapchain_extent,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.preTransform = capabilities.currentTransform,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = presentation_mode,
				.oldSwapchain = VK_NULL_HANDLE,
			};

			VK_CHECK(vkCreateSwapchainKHR(vulkan_core.device, &swapchain_info, nullptr, &vulkan_core.swapchain));
		}

		// create swapchain images
		{
			vkGetSwapchainImagesKHR(vulkan_core.device, vulkan_core.swapchain, &vulkan_core.swapchain_size, nullptr);
			vkGetSwapchainImagesKHR(vulkan_core.device, vulkan_core.swapchain, &vulkan_core.swapchain_size, vulkan_core.swapchain_images);
		}

		// create swapchain views
		{
			for (uint32_t i = 0; i < vulkan_core.swapchain_size; ++i)
			{
				VkImageViewCreateInfo image_view_info
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = vulkan_core.swapchain_images[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = vulkan_core.swapchain_format.format,
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

				VK_CHECK(vkCreateImageView(vulkan_core.device, &image_view_info, nullptr, &vulkan_core.swapchain_views[i]));
			}
		}

		// allocate command buffers
		{
			VkCommandBufferAllocateInfo command_buffer_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = vulkan_core.command_pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = MAX_FRAMES
			};

			VK_CHECK(vkAllocateCommandBuffers(vulkan_core.device, &command_buffer_info, vulkan_core.command_buffers));
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
				VK_CHECK(vkCreateSemaphore(vulkan_core.device, &semaphore_info, nullptr, &vulkan_core.acquire_image[i]));
				VK_CHECK(vkCreateFence(vulkan_core.device, &fence_info, nullptr, &vulkan_core.queue_submit[i]));
			}

			for (uint32_t i = 0; i < vulkan_core.swapchain_size; ++i)
			{
				VK_CHECK(vkCreateSemaphore(vulkan_core.device, &semaphore_info, nullptr, &vulkan_core.present_image[i]));
			}
		}
	}

	void destroy_vulkan_core()
	{
		vkDeviceWaitIdle(vulkan_core.device);

		vkDestroyCommandPool(vulkan_core.device, vulkan_core.command_pool, nullptr);

		for (uint32_t i = 0; i < MAX_FRAMES; ++i)
		{
			vkDestroyFence(vulkan_core.device, vulkan_core.queue_submit[i], nullptr);
			vkDestroySemaphore(vulkan_core.device, vulkan_core.acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < vulkan_core.swapchain_size; ++i)
		{
			vkDestroyImageView(vulkan_core.device, vulkan_core.swapchain_views[i], nullptr);
			vkDestroySemaphore(vulkan_core.device, vulkan_core.present_image[i], nullptr);
		}
		vkDestroySwapchainKHR(vulkan_core.device, vulkan_core.swapchain, nullptr);
		
		vmaDestroyAllocator(vulkan_core.allocator);
		vkDestroyDevice(vulkan_core.device, nullptr);
		vkDestroySurfaceKHR(vulkan_core.instance, vulkan_core.surface, nullptr);
		vkDestroyInstance(vulkan_core.instance, nullptr);
	}

	void recreate_swapchain()
	{
		// Create VkSwapchainKHR
		{
			VkSurfaceFormatKHR preferred_formats[]
			{
				{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
			};

			uint32_t format_count{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_core.gpu, vulkan_core.surface, &format_count, nullptr);
			VkSurfaceFormatKHR formats[15]{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_core.gpu, vulkan_core.surface, &format_count, formats);

			for (uint32_t i = 0; i < format_count; ++i)
			{
				for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
				{
					if (preferred_formats[j].format == formats[i].format &&
						preferred_formats[j].colorSpace == formats[i].colorSpace)
					{
						vulkan_core.swapchain_format = preferred_formats[j];
						break;
					}
				}

				if (vulkan_core.swapchain_format.format != VK_FORMAT_UNDEFINED)
				{
					break;
				}
			}

			if (vulkan_core.swapchain_format.format == VK_FORMAT_UNDEFINED)
			{
				vulkan_core.swapchain_format = formats[0];
			}

			VkSurfaceCapabilitiesKHR capabilities{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_core.gpu, vulkan_core.surface, &capabilities);

			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				vulkan_core.swapchain_extent = capabilities.currentExtent;
			}
			else
			{
				int width, height;
				SDL_GetWindowSizeInPixels(vulkan_core.window, &width, &height);
				vulkan_core.swapchain_extent = { (uint32_t)width, (uint32_t)height };
				SDL_clamp(vulkan_core.swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				SDL_clamp(vulkan_core.swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			}

			uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

			VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

			VkSwapchainCreateInfoKHR swapchain_info
			{
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = vulkan_core.surface,
				.minImageCount = min_image_count,
				.imageFormat = vulkan_core.swapchain_format.format,
				.imageColorSpace = vulkan_core.swapchain_format.colorSpace,
				.imageExtent = vulkan_core.swapchain_extent,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.preTransform = capabilities.currentTransform,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = presentation_mode,
				.oldSwapchain = VK_NULL_HANDLE,
			};

			VK_CHECK(vkCreateSwapchainKHR(vulkan_core.device, &swapchain_info, nullptr, &vulkan_core.swapchain));
		}

		// Swapchain VkImages
		{
			vkGetSwapchainImagesKHR(vulkan_core.device, vulkan_core.swapchain, &vulkan_core.swapchain_size, nullptr);
			vkGetSwapchainImagesKHR(vulkan_core.device, vulkan_core.swapchain, &vulkan_core.swapchain_size, vulkan_core.swapchain_images);
		}

		// Swapchain VkImageViews
		{
			for (uint32_t i = 0; i < vulkan_core.swapchain_size; ++i)
			{
				VkImageViewCreateInfo image_view_info
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = vulkan_core.swapchain_images[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = vulkan_core.swapchain_format.format,
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

				VK_CHECK(vkCreateImageView(vulkan_core.device, &image_view_info, nullptr, &vulkan_core.swapchain_views[i]));
			}
		}
	}

	void begin_frame()
	{
		vkWaitForFences(vulkan_core.device, 1, &vulkan_core.queue_submit[vulkan_core.current_frame], VK_TRUE, UINT64_MAX);
		vkResetFences(vulkan_core.device, 1, &vulkan_core.queue_submit[vulkan_core.current_frame]);

		VkResult acquire_result = vkAcquireNextImageKHR(vulkan_core.device, vulkan_core.swapchain, UINT64_MAX, vulkan_core.acquire_image[vulkan_core.current_frame], VK_NULL_HANDLE, &vulkan_core.image_index);
		if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR)
		{
			printf("Recreating swapchain\n");
			recreate_swapchain();
			return;
		}
		else if (acquire_result != VK_SUCCESS)
		{
			printf("Failed to acquire next image\n");
			abort();
		}

		vkResetCommandBuffer(vulkan_core.command_buffers[vulkan_core.current_frame], 0);
		VkCommandBufferBeginInfo cmd_begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(vulkan_core.command_buffers[vulkan_core.current_frame], &cmd_begin);

		VkImageMemoryBarrier image_barrier_write
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_PIPELINE_STAGE_NONE,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = vulkan_core.swapchain_images[vulkan_core.image_index],
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};

		vkCmdPipelineBarrier(
			vulkan_core.command_buffers[vulkan_core.current_frame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_write);

		VkRenderingAttachmentInfo color_attachment
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = vulkan_core.swapchain_views[vulkan_core.image_index],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {{0.0f, 0.0f, 0.0f, 0.0f}}
		};

		VkRect2D render_area
		{
			.offset = { 0 },
			.extent = vulkan_core.swapchain_extent
		};

		VkRenderingInfo render_info
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = render_area,
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment,
		};

		vkCmdBeginRendering(vulkan_core.command_buffers[vulkan_core.current_frame], &render_info);

		VkViewport viewport
		{
			.width = (float)vulkan_core.swapchain_extent.width,
			.height = (float)vulkan_core.swapchain_extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D scissor
		{
			.offset = {0, 0},
			.extent = vulkan_core.swapchain_extent
		};

		vkCmdSetViewport(vulkan_core.command_buffers[vulkan_core.current_frame], 0, 1, &viewport);
		vkCmdSetScissor(vulkan_core.command_buffers[vulkan_core.current_frame], 0, 1, &scissor);
	}

	void end_frame()
	{
		vkCmdEndRendering(vulkan_core.command_buffers[vulkan_core.current_frame]);

		VkImageMemoryBarrier image_barrier_present
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = vulkan_core.swapchain_images[vulkan_core.image_index],
			.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		};

		vkCmdPipelineBarrier(
			vulkan_core.command_buffers[vulkan_core.current_frame],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_present);

		vkEndCommandBuffer(vulkan_core.command_buffers[vulkan_core.current_frame]);

		VkPipelineStageFlags wait_stages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore wait_semaphores[]{ vulkan_core.acquire_image[vulkan_core.current_frame] };
		VkSemaphore signal_semaphores[]{ vulkan_core.present_image[vulkan_core.current_frame] };

		VkSubmitInfo submit_info
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = wait_semaphores,
			.pWaitDstStageMask = wait_stages,
			.commandBufferCount = 1,
			.pCommandBuffers = &vulkan_core.command_buffers[vulkan_core.current_frame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signal_semaphores
		};

		vkQueueSubmit(vulkan_core.queue, 1, &submit_info, vulkan_core.queue_submit[vulkan_core.current_frame]);

		VkSwapchainKHR swapchains[]{ vulkan_core.swapchain };

		VkPresentInfoKHR present_info
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signal_semaphores,
			.swapchainCount = 1,
			.pSwapchains = swapchains,
			.pImageIndices = &vulkan_core.image_index
		};

		VkResult present_result = vkQueuePresentKHR(vulkan_core.queue, &present_info);
		if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
		{
			printf("recreating swapchain\n");
			recreate_swapchain();
			return;
		}
		else if (present_result != VK_SUCCESS)
		{
			printf("Failed to present image\n");
			return;
		}

		vulkan_core.current_frame = (vulkan_core.current_frame + 1) % MAX_FRAMES;
	}

	vulkan_buffer_t create_vulkan_buffer(VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size)
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

		vulkan_buffer_t buffer{};
		VK_CHECK(vmaCreateBuffer(vulkan_core.allocator, &buffer_create_info, &allocation_create_info, &buffer.buffer, &buffer.allocation, &buffer.info));

		return buffer;
	}

	void destroy_vulkan_buffer(vulkan_buffer_t& buffer)
	{
		vmaDestroyBuffer(vulkan_core.allocator, buffer.buffer, buffer.allocation);
	}


} // olivia