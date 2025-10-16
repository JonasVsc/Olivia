#include "Olivia/Olivia_Renderer_Impl.h"
#include "Olivia/Olivia_Platform_Impl.h"
#include "Olivia/Olivia_Math.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace Olivia
{
	void CreateDevice()
	{
		VulkanCore& core = g_platform->renderer.core;

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
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME
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
			vkCreateInstance(&instance_info, nullptr, &core.instance);
		}

		// Create VkSurfaceKHR
		{
			VkWin32SurfaceCreateInfoKHR win32_surface_info
			{
				.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
				.hinstance = GetModuleHandle(nullptr),
				.hwnd = (HWND)GetActiveWindow()
			};

			VK_CHECK(vkCreateWin32SurfaceKHR(core.instance, &win32_surface_info, nullptr, &core.surface));
		}

		// Create VkDevice
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

		// Create VmaAllocator
		{
			VmaAllocatorCreateInfo vma_info
			{
				.physicalDevice = core.gpu,
				.device = core.device,
				.instance = core.instance,
				.vulkanApiVersion = VK_API_VERSION_1_3
			};

			VK_CHECK(vmaCreateAllocator(&vma_info, &core.allocator));
		}

		// Create VkCommandPool
		{
			VkCommandPoolCreateInfo command_pool_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = core.graphics_queue_index
			};

			VK_CHECK(vkCreateCommandPool(core.device, &command_pool_info, nullptr, &core.command_pool));
		}
	}

	void CreateSwapchain()
	{
		VulkanCore& core = g_platform->renderer.core;

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
				RECT client_rect;
				GetClientRect(GetActiveWindow(), &client_rect);
				uint32_t width = client_rect.right = client_rect.left;
				uint32_t height = client_rect.bottom - client_rect.top;

				core.swapchain_extent.width = Clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				core.swapchain_extent.height = Clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
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

	void CreateSyncObjects()
	{
		VulkanCore& core = g_platform->renderer.core;

		// Allocate VkCommandBuffers
		{
			VkCommandBufferAllocateInfo command_buffer_info
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = core.command_pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = MAX_CONCURRENT_FRAMES
			};

			VK_CHECK(vkAllocateCommandBuffers(core.device, &command_buffer_info, core.command_buffers));

			command_buffer_info.commandBufferCount = 1;

			VK_CHECK(vkAllocateCommandBuffers(core.device, &command_buffer_info, &core.imm_command_buffer));
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
				VK_CHECK(vkCreateSemaphore(core.device, &semaphore_info, nullptr, &core.acquire_image[i]));
				VK_CHECK(vkCreateFence(core.device, &fence_info, nullptr, &core.queue_submit[i]));
			}

			for (uint32_t i = 0; i < core.swapchain_size; ++i)
			{
				VK_CHECK(vkCreateSemaphore(core.device, &semaphore_info, nullptr, &core.present_image[i]));
			}

			VK_CHECK(vkCreateFence(core.device, &fence_info, nullptr, &core.imm_fence));

		}
	}

	void CreateMeshAtlasBuffer()
	{
		VulkanCore& core = g_platform->renderer.core;
		MeshAtlas& mesh_atlas = g_platform->renderer.mesh_atlas;

		mesh_atlas.vertex_buffer = CreateBuffer(
			core.allocator,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			MESH_ATLAS_V_BUFFER_SIZE);

		mesh_atlas.index_buffer = CreateBuffer(
			core.allocator,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			MESH_ATLAS_I_BUFFER_SIZE);

		mesh_atlas.mesh_count = MESH_COUNT;

		// READY-TO-USE MESHES

		Vertex* vertices = (Vertex*)calloc(4, sizeof(Vertex));
		uint32_t* indices = (uint32_t*)calloc(6, sizeof(uint32_t));

		if (vertices && indices)
		{
			// QuadMesh
			{
				uint32_t& v_offset = mesh_atlas.v_offset[QUAD_MESH];
				uint32_t& v_count  = mesh_atlas.v_count[QUAD_MESH];
				uint32_t& i_offset = mesh_atlas.i_offset[QUAD_MESH];
				uint32_t& i_count  = mesh_atlas.i_count[QUAD_MESH];

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

				memcpy(mesh_atlas.vertex_buffer.info.pMappedData, vertices, vertices_size);
				memcpy(mesh_atlas.index_buffer.info.pMappedData, indices, indices_size);

				v_count  = 4;
				i_count  = 6;
				i_offset = 0;
				v_offset = 0;
			}
		}

		free(vertices);
		free(indices);
	}

	void CreatePipelines()
	{
		VulkanCore& core = g_platform->renderer.core;

		VkShaderModule vert = CreateShaderModule(core.device, "olivia.vert.spv");
		VkShaderModule frag = CreateShaderModule(core.device, "olivia.frag.spv");

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
			.pColorAttachmentFormats = &core.swapchain_format.format
		};

		VkPipelineLayoutCreateInfo layout_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		};

		GraphicsPipeline& pipeline = g_platform->renderer.pipelines[OPAQUE_PIPELINE];

		VK_CHECK(vkCreatePipelineLayout(core.device, &layout_info, nullptr, &pipeline.layout));

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

		vkCreateGraphicsPipelines(core.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline.pipeline);

		vkDestroyShaderModule(core.device, vert, nullptr);
		vkDestroyShaderModule(core.device, frag, nullptr);
	}

	void RecreateSwapchain()
	{
		VulkanCore& core = g_platform->renderer.core;

		vkDeviceWaitIdle(core.device);

		// DESTROY OLD SWAPCHAIN & SYNC OBJECTS

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vkDestroySemaphore(core.device, core.acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < core.swapchain_size; ++i)
		{
			vkDestroySemaphore(core.device, core.present_image[i], nullptr);
			vkDestroyFence(core.device, core.queue_submit[i], nullptr);
		}

		for (uint32_t i = 0; i < core.swapchain_size; ++i)
		{
			vkDestroyImageView(core.device, core.swapchain_views[i], nullptr);
		}

		vkDestroySwapchainKHR(core.device, core.swapchain, nullptr);

		core.current_frame = 0;

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
			VK_CHECK(vkCreateSemaphore(core.device, &semaphore_info, nullptr, &core.acquire_image[i]));
			VK_CHECK(vkCreateFence(core.device, &fence_info, nullptr, &core.queue_submit[i]));
		}

		for (uint32_t i = 0; i < core.swapchain_size; ++i)
		{
			VK_CHECK(vkCreateSemaphore(core.device, &semaphore_info, nullptr, &core.present_image[i]));
		}

		// RECREATE SWAPCHAIN
		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(core.gpu, core.surface, &capabilities);

		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			core.swapchain_extent = capabilities.currentExtent;
		}
		else
		{
			RECT client_rect;
			GetClientRect(GetActiveWindow(), &client_rect);
			uint32_t width = client_rect.right = client_rect.left;
			uint32_t height = client_rect.bottom - client_rect.top;

			core.swapchain_extent.width = Clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			core.swapchain_extent.height = Clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
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

		vkGetSwapchainImagesKHR(core.device, core.swapchain, &core.swapchain_size, nullptr);
		vkGetSwapchainImagesKHR(core.device, core.swapchain, &core.swapchain_size, core.swapchain_images);

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

	Buffer CreateBuffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size)
	{
		VulkanCore& core = g_platform->renderer.core;

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
		VK_CHECK(vmaCreateBuffer(core.allocator, &buffer_create_info, &allocation_create_info, &buffer.buffer, &buffer.allocation, &buffer.info));

		return buffer;
	}

	VkShaderModule CreateShaderModule(VkDevice device, const char* shader_path)
	{
		size_t code_size;
		uint8_t* shader_code = (uint8_t*)ReadFileFromPath(shader_path, &code_size);

		VkShaderModuleCreateInfo shader_info
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = code_size,
			.pCode = (const uint32_t*)shader_code
		};

		VkShaderModule shader_module{ VK_NULL_HANDLE };
		VK_CHECK(vkCreateShaderModule(device, &shader_info, nullptr, &shader_module));

		FreeFile(shader_code);
		return shader_module;
	}

	void BeginFrame()
	{
		if (!Running()) return;

		VulkanCore& core = g_platform->renderer.core;

		vkWaitForFences(core.device, 1, &core.queue_submit[core.current_frame], VK_TRUE, UINT64_MAX);
		vkResetFences(core.device, 1, &core.queue_submit[core.current_frame]);

		VkResult acquire_result = vkAcquireNextImageKHR(core.device, core.swapchain, UINT64_MAX, core.acquire_image[core.current_frame], VK_NULL_HANDLE, &core.image_index);
		if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR)
		{
			LOG_INFO(TAG_RENDERER, "Recreating swapchain");
			RecreateSwapchain();
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

	void EndFrame()
	{
		if (!Running()) return;

		VulkanCore& core = g_platform->renderer.core;

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
			RecreateSwapchain();
			return;
		}
		else if (present_result != VK_SUCCESS)
		{
			LOG_ERROR(TAG_RENDERER, "Failed to present image");
			return;
		}

		core.current_frame = (core.current_frame + 1) % MAX_CONCURRENT_FRAMES;
	}

	void DestroyRenderer()
	{
		VulkanCore& core = g_platform->renderer.core;
		MeshAtlas& mesh_atlas = g_platform->renderer.mesh_atlas;

		vkDeviceWaitIdle(core.device);

		// DESTROY MESH ATLAS

		vmaDestroyBuffer(core.allocator, mesh_atlas.vertex_buffer.buffer, mesh_atlas.vertex_buffer.allocation);
		vmaDestroyBuffer(core.allocator, mesh_atlas.index_buffer.buffer, mesh_atlas.index_buffer.allocation);

		// DESTROY CORE

		vkDestroyFence(core.device, core.imm_fence, nullptr);

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i)
		{
			vkDestroySemaphore(core.device, core.acquire_image[i], nullptr);
		}

		for (uint32_t i = 0; i < core.swapchain_size; ++i)
		{
			vkDestroyImageView(core.device, core.swapchain_views[i], nullptr);
			vkDestroySemaphore(core.device, core.present_image[i], nullptr);
			vkDestroyFence(core.device, core.queue_submit[i], nullptr);
		}

		vkDestroyCommandPool(core.device, core.command_pool, nullptr);
		vkDestroySwapchainKHR(core.device, core.swapchain, nullptr);
		vmaDestroyAllocator(core.allocator);

		vkDestroyDevice(core.device, nullptr);
		vkDestroySurfaceKHR(core.instance, core.surface, nullptr);
		vkDestroyInstance(core.instance, nullptr);
	}
}