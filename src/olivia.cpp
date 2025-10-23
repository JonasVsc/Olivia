#include "olivia/olivia.h"

#define VMA_IMPLEMENTATION
#include<vma/vk_mem_alloc.h>

context_t* g_context{ nullptr };

void init_vulkan_core()
{
	context_t& ctx = *g_context;

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

		VK_CHECK(vkCreateInstance(&instance_info, nullptr, &ctx.instance));
	}

	// create surface
	{
		bool res = SDL_Vulkan_CreateSurface(ctx.window, ctx.instance, nullptr, &ctx.surface);
		if (!res)
		{
			printf("SDL_Error: %s\n", SDL_GetError());
			abort();
		}
	}

	// create device
	{
		uint32_t gpu_count{};
		vkEnumeratePhysicalDevices(ctx.instance, &gpu_count, nullptr);

		if (gpu_count <= 0)
		{

			printf("No suitable gpu found\n");
			abort();
		}

		VkPhysicalDevice gpus[10]{};
		vkEnumeratePhysicalDevices(ctx.instance, &gpu_count, gpus);

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

			ctx.graphics_queue_index = UINT32_MAX;

			for (uint32_t i = 0; i < queue_family_count; ++i)
			{
				VkBool32 support_presentation{ VK_FALSE };
				vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], i, ctx.surface, &support_presentation);

				if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && support_presentation)
				{
					ctx.graphics_queue_index = i;
					ctx.gpu = gpus[i];
					break;
				}
			}
		}

		if (ctx.graphics_queue_index == UINT32_MAX)
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
				.queueFamilyIndex = ctx.graphics_queue_index,
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

		VK_CHECK(vkCreateDevice(ctx.gpu, &device_info, nullptr, &ctx.device));

		vkGetDeviceQueue(ctx.device, ctx.graphics_queue_index, 0, &ctx.queue);
	}

	// create command_pool
	{
		VkCommandPoolCreateInfo command_pool_info
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = ctx.graphics_queue_index
		};

		VK_CHECK(vkCreateCommandPool(ctx.device, &command_pool_info, nullptr, &ctx.command_pool));
	}

	// create swapchain
	{
		VkSurfaceFormatKHR preferred_formats[]
		{
			{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
		};

		uint32_t format_count{};
		vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.gpu, ctx.surface, &format_count, nullptr);
		VkSurfaceFormatKHR formats[15]{};
		vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.gpu, ctx.surface, &format_count, formats);

		for (uint32_t i = 0; i < format_count; ++i)
		{
			for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
			{
				if (preferred_formats[j].format == formats[i].format &&
					preferred_formats[j].colorSpace == formats[i].colorSpace)
				{
					ctx.swapchain_format = preferred_formats[j];
					break;
				}
			}

			if (ctx.swapchain_format.format != VK_FORMAT_UNDEFINED)
			{
				break;
			}
		}

		if (ctx.swapchain_format.format == VK_FORMAT_UNDEFINED)
		{
			ctx.swapchain_format = formats[0];
		}

		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.gpu, ctx.surface, &capabilities);

		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			ctx.swapchain_extent = capabilities.currentExtent;
		}
		else
		{
			int width, height;
			SDL_GetWindowSizeInPixels(ctx.window, &width, &height);
			ctx.swapchain_extent = { (uint32_t)width, (uint32_t)height };
			SDL_clamp(ctx.swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			SDL_clamp(ctx.swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}

		uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

		VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

		VkSwapchainCreateInfoKHR swapchain_info
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = ctx.surface,
			.minImageCount = min_image_count,
			.imageFormat = ctx.swapchain_format.format,
			.imageColorSpace = ctx.swapchain_format.colorSpace,
			.imageExtent = ctx.swapchain_extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentation_mode,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		VK_CHECK(vkCreateSwapchainKHR(ctx.device, &swapchain_info, nullptr, &ctx.swapchain));
	}

	// create swapchain images
	{
		vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &ctx.swapchain_size, nullptr);
		vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &ctx.swapchain_size, ctx.swapchain_images);
	}

	// create swapchain views
	{
		for (uint32_t i = 0; i < ctx.swapchain_size; ++i)
		{
			VkImageViewCreateInfo image_view_info
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = ctx.swapchain_images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = ctx.swapchain_format.format,
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

			VK_CHECK(vkCreateImageView(ctx.device, &image_view_info, nullptr, &ctx.swapchain_views[i]));
		}
	}

	// allocate command buffers
	{
		VkCommandBufferAllocateInfo command_buffer_info
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = ctx.command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = MAX_FRAMES
		};

		VK_CHECK(vkAllocateCommandBuffers(ctx.device, &command_buffer_info, ctx.command_buffers));
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
			VK_CHECK(vkCreateSemaphore(ctx.device, &semaphore_info, nullptr, &ctx.acquire_image[i]));
			VK_CHECK(vkCreateFence(ctx.device, &fence_info, nullptr, &ctx.queue_submit[i]));
		}

		for (uint32_t i = 0; i < ctx.swapchain_size; ++i)
		{
			VK_CHECK(vkCreateSemaphore(ctx.device, &semaphore_info, nullptr, &ctx.present_image[i]));
		}
	}
}

void init_pipelines()
{
	context_t& ctx = *g_context;

	VkVertexInputBindingDescription vertex_bindings_2d[]
	{
		// Binding point 0: Mesh vertex layout description at per-vertex rate
		{
			.binding = 0,
			.stride = sizeof(vertex2d_t),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		},

		// Binding point 1: Instanced data at per-instance rate
		{
			.binding = 1,
			.stride = sizeof(instance2d_t),
			.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
		}
	};

	VkVertexInputBindingDescription vertex_bindings_3d[]
	{
		// Binding point 0: Mesh vertex layout description at per-vertex rate
		{
			.binding = 0,
			.stride = sizeof(vertex3d_t),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		},

		// Binding point 1: Instanced data at per-instance rate
		{
			.binding = 1,
			.stride = sizeof(instance3d_t),
			.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
		}
	};

	VkVertexInputAttributeDescription vertex_attributes_2d[]
	{
		// Per-vertex attributes
		{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex2d_t, position)},
		{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex2d_t, normal)},

		// Per-instance attributes
		{2, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},
		{3, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3},
		{4, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 6},
	};

	VkVertexInputAttributeDescription vertex_attributes_3d[]
	{
		// Per-vertex attributes
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex3d_t, position)},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex3d_t, normal)},
		{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex3d_t, uv)},

		// Per-instance attributes
		{3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
		{4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 4},
		{5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 8},
		{6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 12},
	};

	// --- 2D ---

	ctx.pipelines[PIPELINE_2D] = build_pipeline({
		.vertex_shader_path = "olivia2d.vert.spv",
		.frag_shader_path = "olivia2d.frag.spv",
		.vertex_binding_count = ARRAY_SIZE(vertex_bindings_2d),
		.vertex_bindings = vertex_bindings_2d,
		.vertex_attribute_count = ARRAY_SIZE(vertex_attributes_2d),
		.vertex_attributes = vertex_attributes_2d,
	});

	// --- 3D ---

	ctx.pipelines[PIPELINE_3D] = build_pipeline({
		.vertex_shader_path = "olivia3d.vert.spv",
		.frag_shader_path = "olivia3d.frag.spv",
		.vertex_binding_count = ARRAY_SIZE(vertex_bindings_3d),
		.vertex_bindings = vertex_bindings_3d,
		.vertex_attribute_count = ARRAY_SIZE(vertex_attributes_3d),
		.vertex_attributes = vertex_attributes_3d
	});
}

void begin_frame()
{
	context_t& ctx = *g_context;

	vkWaitForFences(ctx.device, 1, &ctx.queue_submit[ctx.current_frame], VK_TRUE, UINT64_MAX);
	vkResetFences(ctx.device, 1, &ctx.queue_submit[ctx.current_frame]);

	VkResult acquire_result = vkAcquireNextImageKHR(ctx.device, ctx.swapchain, UINT64_MAX, ctx.acquire_image[ctx.current_frame], VK_NULL_HANDLE, &ctx.image_index);
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

	vkResetCommandBuffer(ctx.command_buffers[ctx.current_frame], 0);
	VkCommandBufferBeginInfo cmd_begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(ctx.command_buffers[ctx.current_frame], &cmd_begin);

	VkImageMemoryBarrier image_barrier_write
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_PIPELINE_STAGE_NONE,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = ctx.swapchain_images[ctx.image_index],
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};

	vkCmdPipelineBarrier(
		ctx.command_buffers[ctx.current_frame],
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &image_barrier_write);

	VkRenderingAttachmentInfo color_attachment
	{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = ctx.swapchain_views[ctx.image_index],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {{0.0f, 0.0f, 0.0f, 0.0f}}
	};

	VkRect2D render_area
	{
		.offset = { 0 },
		.extent = ctx.swapchain_extent
	};

	VkRenderingInfo render_info
	{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = render_area,
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment,
	};

	vkCmdBeginRendering(ctx.command_buffers[ctx.current_frame], &render_info);

	VkViewport viewport
	{
		.width = (float)ctx.swapchain_extent.width,
		.height = (float)ctx.swapchain_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor
	{
		.offset = {0, 0},
		.extent = ctx.swapchain_extent
	};

	vkCmdSetViewport(ctx.command_buffers[ctx.current_frame], 0, 1, &viewport);
	vkCmdSetScissor(ctx.command_buffers[ctx.current_frame], 0, 1, &scissor);
}

void end_frame()
{
	context_t& ctx = *g_context;

	vkCmdEndRendering(ctx.command_buffers[ctx.current_frame]);

	VkImageMemoryBarrier image_barrier_present
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = 0,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = ctx.swapchain_images[ctx.image_index],
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};

	vkCmdPipelineBarrier(
		ctx.command_buffers[ctx.current_frame],
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &image_barrier_present);

	vkEndCommandBuffer(ctx.command_buffers[ctx.current_frame]);

	VkPipelineStageFlags wait_stages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore wait_semaphores[]{ ctx.acquire_image[ctx.current_frame] };
	VkSemaphore signal_semaphores[]{ ctx.present_image[ctx.current_frame] };

	VkSubmitInfo submit_info
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &ctx.command_buffers[ctx.current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphores
	};

	vkQueueSubmit(ctx.queue, 1, &submit_info, ctx.queue_submit[ctx.current_frame]);

	VkSwapchainKHR swapchains[]{ ctx.swapchain };

	VkPresentInfoKHR present_info
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = swapchains,
		.pImageIndices = &ctx.image_index
	};

	VkResult present_result = vkQueuePresentKHR(ctx.queue, &present_info);
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

	ctx.current_frame = (ctx.current_frame + 1) % MAX_FRAMES;
}

int main(int argc, char* args[])
{
	// --- init context ---

	g_context = (context_t*)calloc(1, sizeof(context_t));

	if (!g_context)
	{
		printf("failed to init context\n");
		return -1;
	}
	
	context_t& ctx = *g_context;

	// --- window ---

	ctx.window = SDL_CreateWindow("olivia", 640, 480, SDL_WINDOW_VULKAN);

	if (!ctx.window)
	{
		printf("failed to create window\n");
		return -1;
	}

	// --- vulkan core ---

	init_vulkan_core();

	// --- renderer resources ---

	init_pipelines();

	// --- run ---

	bool running = true;
	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}

			begin_frame();

			end_frame();
		}
	}

	return 0;
}

VkShaderModule create_shader_module(VkDevice device, const char* shader_path)
{
	SDL_IOStream* io = SDL_IOFromFile(shader_path, "rb");
	if (io)
	{
		size_t size = SDL_GetIOSize(io);

		uint8_t* shader_code = (uint8_t*)SDL_calloc(1, size);
		if (shader_code)
		{
			while (SDL_ReadIO(io, shader_code, size) != size);
			SDL_CloseIO(io);

			VkShaderModuleCreateInfo shader_info
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = size,
				.pCode = (const uint32_t*)shader_code
			};

			VkShaderModule shader_module{ VK_NULL_HANDLE };
			VK_CHECK(vkCreateShaderModule(device, &shader_info, nullptr, &shader_module));

			SDL_free(shader_code);

			return shader_module;
		}
	}

	return nullptr;
}

pipeline_t build_pipeline(const pipeline_info_t& info)
{
	context_t& ctx = *g_context;

	VkShaderModule vert = create_shader_module(ctx.device, info.vertex_shader_path);
	VkShaderModule frag = create_shader_module(ctx.device, info.frag_shader_path);

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

	VkPipelineVertexInputStateCreateInfo vertex_input_state_info
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = info.vertex_binding_count,
		.pVertexBindingDescriptions = info.vertex_bindings,
		.vertexAttributeDescriptionCount = info.vertex_attribute_count,
		.pVertexAttributeDescriptions = info.vertex_attributes
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
		.pColorAttachmentFormats = &ctx.swapchain_format.format
	};

	VkPipelineLayoutCreateInfo layout_info
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = info.set_layout_count,
		.pSetLayouts = info.set_layouts
	};

	pipeline_t pipeline{};

	VK_CHECK(vkCreatePipelineLayout(ctx.device, &layout_info, nullptr, &pipeline.layout));

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
		.layout = pipeline.layout,
		.renderPass = VK_NULL_HANDLE,
		.subpass = 0
	};

	VK_CHECK(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline.pipeline));

	vkDestroyShaderModule(ctx.device, vert, nullptr);
	vkDestroyShaderModule(ctx.device, frag, nullptr);

	return pipeline;
}

void recreate_swapchain()
{
	context_t& ctx = *g_context;

	// Create VkSwapchainKHR
	{
		VkSurfaceFormatKHR preferred_formats[]
		{
			{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
		};

		uint32_t format_count{};
		vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.gpu, ctx.surface, &format_count, nullptr);
		VkSurfaceFormatKHR formats[15]{};
		vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.gpu, ctx.surface, &format_count, formats);

		for (uint32_t i = 0; i < format_count; ++i)
		{
			for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
			{
				if (preferred_formats[j].format == formats[i].format &&
					preferred_formats[j].colorSpace == formats[i].colorSpace)
				{
					ctx.swapchain_format = preferred_formats[j];
					break;
				}
			}

			if (ctx.swapchain_format.format != VK_FORMAT_UNDEFINED)
			{
				break;
			}
		}

		if (ctx.swapchain_format.format == VK_FORMAT_UNDEFINED)
		{
			ctx.swapchain_format = formats[0];
		}

		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.gpu, ctx.surface, &capabilities);

		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			ctx.swapchain_extent = capabilities.currentExtent;
		}
		else
		{
			int width, height;
			SDL_GetWindowSizeInPixels(ctx.window, &width, &height);
			ctx.swapchain_extent = { (uint32_t)width, (uint32_t)height };
			SDL_clamp(ctx.swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			SDL_clamp(ctx.swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}

		uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

		VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

		VkSwapchainCreateInfoKHR swapchain_info
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = ctx.surface,
			.minImageCount = min_image_count,
			.imageFormat = ctx.swapchain_format.format,
			.imageColorSpace = ctx.swapchain_format.colorSpace,
			.imageExtent = ctx.swapchain_extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentation_mode,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		VK_CHECK(vkCreateSwapchainKHR(ctx.device, &swapchain_info, nullptr, &ctx.swapchain));
	}

	// Swapchain VkImages
	{
		vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &ctx.swapchain_size, nullptr);
		vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &ctx.swapchain_size, ctx.swapchain_images);
	}

	// Swapchain VkImageViews
	{
		for (uint32_t i = 0; i < ctx.swapchain_size; ++i)
		{
			VkImageViewCreateInfo image_view_info
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = ctx.swapchain_images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = ctx.swapchain_format.format,
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

			VK_CHECK(vkCreateImageView(ctx.device, &image_view_info, nullptr, &ctx.swapchain_views[i]));
		}
	}
}