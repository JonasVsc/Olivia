#include "Olivia/Olivia_Graphics.h"

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace Olivia
{

	VulkanCore::VulkanCore(SDL_Window& window)
		: m_window{window}
	{
		create_device();
		create_swapchain();
		create_sync_objects();
	}

	VulkanCore::~VulkanCore()
	{
		vkDestroyFence(m_device, m_imm_fence, nullptr);

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vkDestroySemaphore(m_device, m_acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < m_swapchain_size; ++i)
		{
			vkDestroyImageView(m_device, m_swapchain_views[i], nullptr);
			vkDestroySemaphore(m_device, m_present_image[i], nullptr);
			vkDestroyFence(m_device, m_queue_submit[i], nullptr);
		}

		vkDestroyCommandPool(m_device, m_command_pool, nullptr);
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		vmaDestroyAllocator(m_allocator);

		vkDestroyDevice(m_device, nullptr);
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);
	}

	void VulkanCore::recreate_swapchain()
	{
		vkDeviceWaitIdle(m_device);

		// DESTROY OLD SWAPCHAIN & SYNC OBJECTS

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vkDestroySemaphore(m_device, m_acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < m_swapchain_size; ++i)
		{
			vkDestroySemaphore(m_device, m_present_image[i], nullptr);
			vkDestroyFence(m_device, m_queue_submit[i], nullptr);
		}

		for (uint32_t i = 0; i < m_swapchain_size; ++i)
		{
			vkDestroyImageView(m_device, m_swapchain_views[i], nullptr);
		}

		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

		m_current_frame = 0;

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
			VK_CHECK(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_acquire_image[i]));
			VK_CHECK(vkCreateFence(m_device, &fence_info, nullptr, &m_queue_submit[i]));
		}

		for (uint32_t i = 0; i < m_swapchain_size; ++i)
		{
			VK_CHECK(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_present_image[i]));
		}

		// RECREATE SWAPCHAIN
		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu, m_surface, &capabilities);

		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			m_swapchain_extent = capabilities.currentExtent;
		}
		else
		{
			int width, height;
			SDL_GetWindowSizeInPixels(&m_window, &width, & height);
			m_swapchain_extent = { (uint32_t)width, (uint32_t)height };
			SDL_clamp(m_swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			SDL_clamp(m_swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}

		uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

		VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

		VkSwapchainCreateInfoKHR swapchain_info
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = m_surface,
			.minImageCount = min_image_count,
			.imageFormat = m_swapchain_format.format,
			.imageColorSpace = m_swapchain_format.colorSpace,
			.imageExtent = m_swapchain_extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentation_mode,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		VK_CHECK(vkCreateSwapchainKHR(m_device, &swapchain_info, nullptr, &m_swapchain));

		vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchain_size, nullptr);
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchain_size, m_swapchain_images);

		for (uint32_t i = 0; i < m_swapchain_size; ++i)
		{
			VkImageViewCreateInfo image_view_info
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = m_swapchain_images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_swapchain_format.format,
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

			VK_CHECK(vkCreateImageView(m_device, &image_view_info, nullptr, &m_swapchain_views[i]));
		}
	}

	void VulkanCore::create_device()
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

			vkCreateInstance(&instance_info, nullptr, &m_instance);
		}

		// Create VkSurfaceKHR
		bool res = SDL_Vulkan_CreateSurface(&m_window, m_instance, nullptr, &m_surface);

		// Create VkDevice
		{
			uint32_t gpu_count{};
			vkEnumeratePhysicalDevices(m_instance, &gpu_count, nullptr);

			if (gpu_count <= 0)
			{

				printf("No suitable gpu found\n");
				abort();
			}

			VkPhysicalDevice gpus[10]{};
			vkEnumeratePhysicalDevices(m_instance, &gpu_count, gpus);

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

				m_graphics_queue_index = UINT32_MAX;

				for (uint32_t i = 0; i < queue_family_count; ++i)
				{
					VkBool32 support_presentation{ VK_FALSE };
					vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], i, m_surface, &support_presentation);

					if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && support_presentation)
					{
						m_graphics_queue_index = i;
						m_gpu = gpus[i];
						break;
					}
				}
			}

			if (m_graphics_queue_index == UINT32_MAX)
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
					.queueFamilyIndex = m_graphics_queue_index,
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

			VK_CHECK(vkCreateDevice(m_gpu, &device_info, nullptr, &m_device));

			vkGetDeviceQueue(m_device, m_graphics_queue_index, 0, &m_queue);
		}

		// Create VmaAllocator
		{
			VmaAllocatorCreateInfo vma_info
			{
				.physicalDevice = m_gpu,
				.device = m_device,
				.instance = m_instance,
				.vulkanApiVersion = VK_API_VERSION_1_3
			};

			VK_CHECK(vmaCreateAllocator(&vma_info, &m_allocator));
		}

		// Create VkCommandPool
		{
			VkCommandPoolCreateInfo command_pool_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = m_graphics_queue_index
			};

			VK_CHECK(vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_command_pool));
		}
	}

	void VulkanCore::create_swapchain()
	{
		// Create VkSwapchainKHR
		{
			VkSurfaceFormatKHR preferred_formats[]
			{
				{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
			};

			uint32_t format_count{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &format_count, nullptr);
			VkSurfaceFormatKHR formats[15]{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &format_count, formats);

			for (uint32_t i = 0; i < format_count; ++i)
			{
				for (uint32_t j = 0; j < sizeof(preferred_formats[0]) / sizeof(preferred_formats); ++j)
				{
					if (preferred_formats[j].format == formats[i].format &&
						preferred_formats[j].colorSpace == formats[i].colorSpace)
					{
						m_swapchain_format = preferred_formats[j];
						break;
					}
				}

				if (m_swapchain_format.format != VK_FORMAT_UNDEFINED)
				{
					break;
				}
			}

			if (m_swapchain_format.format == VK_FORMAT_UNDEFINED)
			{
				m_swapchain_format = formats[0];
			}

			VkSurfaceCapabilitiesKHR capabilities{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu, m_surface, &capabilities);

			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				m_swapchain_extent = capabilities.currentExtent;
			}
			else
			{
				int width, height;
				SDL_GetWindowSizeInPixels(&m_window, &width, &height);
				m_swapchain_extent = { (uint32_t)width, (uint32_t)height };
				SDL_clamp(m_swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				SDL_clamp(m_swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			}

			uint32_t min_image_count = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;

			VkPresentModeKHR presentation_mode = VK_PRESENT_MODE_FIFO_KHR;

			VkSwapchainCreateInfoKHR swapchain_info
			{
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = m_surface,
				.minImageCount = min_image_count,
				.imageFormat = m_swapchain_format.format,
				.imageColorSpace = m_swapchain_format.colorSpace,
				.imageExtent = m_swapchain_extent,
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.preTransform = capabilities.currentTransform,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = presentation_mode,
				.oldSwapchain = VK_NULL_HANDLE,
			};

			VK_CHECK(vkCreateSwapchainKHR(m_device, &swapchain_info, nullptr, &m_swapchain));
		}

		// Swapchain VkImages
		{
			vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchain_size, nullptr);
			vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchain_size, m_swapchain_images);
		}

		// Swapchain VkImageViews
		{
			for (uint32_t i = 0; i < m_swapchain_size; ++i)
			{
				VkImageViewCreateInfo image_view_info
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = m_swapchain_images[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = m_swapchain_format.format,
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

				VK_CHECK(vkCreateImageView(m_device, &image_view_info, nullptr, &m_swapchain_views[i]));
			}
		}
	}

	void VulkanCore::create_sync_objects()
	{
		// Allocate VkCommandBuffers
		{
			VkCommandBufferAllocateInfo command_buffer_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = m_command_pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = MAX_CONCURRENT_FRAMES
			};

			VK_CHECK(vkAllocateCommandBuffers(m_device, &command_buffer_info, m_command_buffers));

			command_buffer_info.commandBufferCount = 1;

			VK_CHECK(vkAllocateCommandBuffers(m_device, &command_buffer_info, &m_imm_command_buffer));
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
				VK_CHECK(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_acquire_image[i]));
				VK_CHECK(vkCreateFence(m_device, &fence_info, nullptr, &m_queue_submit[i]));
			}

			for (uint32_t i = 0; i < m_swapchain_size; ++i)
			{
				VK_CHECK(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_present_image[i]));
			}

			VK_CHECK(vkCreateFence(m_device, &fence_info, nullptr, &m_imm_fence));
		}
	}

	Pipeline::Pipeline(VulkanCore& core)
		: m_core{core}
	{
		create_pipeline({
			.vertex_shader_path = "olivia.vert.spv",
			.fragment_shader_path = "olivia.frag.spv"
			}, PIPELINE_OPAQUE);
	}

	Pipeline::~Pipeline()
	{
		for (uint32_t i = 0; i < PIPELINE_MAX; ++i)
		{
			vkDestroyPipelineLayout(m_core.get_device(), m_layout[i], nullptr);
			vkDestroyPipeline(m_core.get_device(), m_pipeline[i], nullptr);
		}
	}

	void Pipeline::create_pipeline(const PipelineInfo& info, PipelineType type)
	{
		VkShaderModule vert = create_shader_module(m_core.get_device(), info.vertex_shader_path);
		VkShaderModule frag = create_shader_module(m_core.get_device(), info.fragment_shader_path);

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
				.stride = sizeof(Instance),
				.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
			}
		};

		VkVertexInputAttributeDescription vertex_attributes[]
		{
			// Per-vertex attributes
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
			{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},

			// Per-instance attributes
			{3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, transform[0])},
			{4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, transform[4])},
			{5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, transform[8])},
			{6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, transform[12])},
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
			.pColorAttachmentFormats = &m_core.m_swapchain_format.format
		};

		VkPipelineLayoutCreateInfo layout_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		};

		VK_CHECK(vkCreatePipelineLayout(m_core.get_device(), &layout_info, nullptr, &m_layout[type]));

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
			.layout = m_layout[type],
			.renderPass = VK_NULL_HANDLE,
			.subpass = 0
		};

		vkCreateGraphicsPipelines(m_core.get_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_pipeline[type]);

		vkDestroyShaderModule(m_core.get_device(), vert, nullptr);
		vkDestroyShaderModule(m_core.get_device(), frag, nullptr);
	}

	VkShaderModule Pipeline::create_shader_module(VkDevice device, const char* shader_path)
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

	Mesh::Mesh(VulkanCore& core)
		: m_core{core}
	{
		m_vertex_buffer = create_buffer(
			m_core.get_allocator(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			MEGABYTES(100));

		m_index_buffer = create_buffer(
			m_core.get_allocator(),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			MEGABYTES(100));

		// QUAD MESH
		{
			Vertex* vertices = (Vertex*)calloc(4, sizeof(Vertex));
			uint32_t* indices = (uint32_t*)calloc(6, sizeof(uint32_t));

			if (vertices && indices)
			{
				vertices[0] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 0.0f, 0.0f } };
				vertices[1] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 1.0f, 0.0f } };
				vertices[2] = { { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 1.0f, 1.0f } };
				vertices[3] = { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 0.0f, 1.0f } };

				indices[0] = 0;
				indices[1] = 1;
				indices[2] = 2;
				indices[3] = 2;
				indices[4] = 3;
				indices[5] = 0;

				uint32_t vertices_size = 4 * sizeof(Vertex);
				uint32_t indices_size = 6 * sizeof(uint32_t);

				memcpy(m_vertex_buffer.info.pMappedData, vertices, vertices_size);
				memcpy(m_index_buffer.info.pMappedData, indices, indices_size);
			
				m_mesh[MESH_QUAD].vertex_count  = 4;
				m_mesh[MESH_QUAD].index_count   = 6;
				m_mesh[MESH_QUAD].vertex_offset = 0;
				m_mesh[MESH_QUAD].index_offset  = 0;
			}
		}
	}

	Mesh::~Mesh()
	{
		vmaDestroyBuffer(m_core.get_allocator(), m_vertex_buffer.buffer, m_vertex_buffer.allocation);
		vmaDestroyBuffer(m_core.get_allocator(), m_index_buffer.buffer, m_index_buffer.allocation);
	}

	void Mesh::upload_mesh(const MeshInfo& info, MeshType type)
	{

	}


	Renderer::Renderer(SDL_Window& window)
		: m_window{window}
	{
	}

	Renderer::~Renderer()
	{
		vkDeviceWaitIdle(m_core.get_device());
	}

	void Renderer::begin_frame()
	{
		vkWaitForFences(m_core.m_device, 1, &m_core.m_queue_submit[m_core.m_current_frame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_core.m_device, 1, &m_core.m_queue_submit[m_core.m_current_frame]);

		VkResult acquire_result = vkAcquireNextImageKHR(m_core.m_device, m_core.m_swapchain, UINT64_MAX, m_core.m_acquire_image[m_core.m_current_frame], VK_NULL_HANDLE, &m_core.m_image_index);
		if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR)
		{
			LOG_INFO(TAG_RENDERER, "Recreating swapchain");
			m_core.recreate_swapchain();
			return;
		}
		else if (acquire_result != VK_SUCCESS)
		{
			LOG_ERROR(TAG_RENDERER, "Failed to acquire next image");
			abort();
		}

		vkResetCommandBuffer(m_core.m_command_buffers[m_core.m_current_frame], 0);
		VkCommandBufferBeginInfo cmd_begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(m_core.m_command_buffers[m_core.m_current_frame], &cmd_begin);

		VkImageMemoryBarrier image_barrier_write
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_PIPELINE_STAGE_NONE,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = m_core.m_swapchain_images[m_core.m_image_index],
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};

		vkCmdPipelineBarrier(
			m_core.m_command_buffers[m_core.m_current_frame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_write);

		VkRenderingAttachmentInfo color_attachment
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = m_core.m_swapchain_views[m_core.m_image_index],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {{0.0f, 0.0f, 0.0f, 0.0f}}
		};

		VkRect2D render_area
		{
			.offset = { 0 },
			.extent = m_core.m_swapchain_extent
		};

		VkRenderingInfo render_info
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = render_area,
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment,
		};

		vkCmdBeginRendering(m_core.m_command_buffers[m_core.m_current_frame], &render_info);

		VkViewport viewport
		{
			.width = (float)m_core.m_swapchain_extent.width,
			.height = (float)m_core.m_swapchain_extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D scissor
		{
			.offset = {0, 0},
			.extent = m_core.m_swapchain_extent
		};

		vkCmdSetViewport(m_core.m_command_buffers[m_core.m_current_frame], 0, 1, &viewport);
		vkCmdSetScissor(m_core.m_command_buffers[m_core.m_current_frame], 0, 1, &scissor);
	}

	void Renderer::end_frame()
	{

		vkCmdEndRendering(m_core.m_command_buffers[m_core.m_current_frame]);

		VkImageMemoryBarrier image_barrier_present
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = m_core.m_swapchain_images[m_core.m_image_index],
			.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		};

		vkCmdPipelineBarrier(
			m_core.m_command_buffers[m_core.m_current_frame],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &image_barrier_present);

		vkEndCommandBuffer(m_core.m_command_buffers[m_core.m_current_frame]);

		VkPipelineStageFlags wait_stages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore wait_semaphores[]{ m_core.m_acquire_image[m_core.m_current_frame] };
		VkSemaphore signal_semaphores[]{ m_core.m_present_image[m_core.m_current_frame] };

		VkSubmitInfo submit_info
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = wait_semaphores,
			.pWaitDstStageMask = wait_stages,
			.commandBufferCount = 1,
			.pCommandBuffers = &m_core.m_command_buffers[m_core.m_current_frame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signal_semaphores
		};

		vkQueueSubmit(m_core.m_queue, 1, &submit_info, m_core.m_queue_submit[m_core.m_current_frame]);

		VkSwapchainKHR swapchains[]{ m_core.m_swapchain };

		VkPresentInfoKHR present_info
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signal_semaphores,
			.swapchainCount = 1,
			.pSwapchains = swapchains,
			.pImageIndices = &m_core.m_image_index
		};

		VkResult present_result = vkQueuePresentKHR(m_core.m_queue, &present_info);
		if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
		{
			LOG_INFO(TAG_RENDERER, "Recreating swapchain");
			m_core.recreate_swapchain();
			return;
		}
		else if (present_result != VK_SUCCESS)
		{
			LOG_ERROR(TAG_RENDERER, "Failed to present image");
			return;
		}

		m_core.m_current_frame = (m_core.m_current_frame + 1) % MAX_CONCURRENT_FRAMES;
	}

	Buffer create_buffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size)
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

		Buffer buffer{};
		VK_CHECK(vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &buffer.buffer, &buffer.allocation, &buffer.info));

		return buffer;
	}

} // Olivia