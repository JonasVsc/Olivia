#include "olivia/graphics/olivia_graphics_internal.h"
#include "olivia/renderer/olivia_renderer_internal.h"

namespace olivia
{
	graphics_allocator local_graphics_allocator;

	bool init_graphics_allocator(lifetime_allocator allocator, renderer renderer)
	{
		local_graphics_allocator = (graphics_allocator)allocate(allocator, sizeof(graphics_allocator_t));

		if (local_graphics_allocator)
		{
			vulkan_core_t& core = renderer->core;
			
			VmaAllocatorCreateInfo vma_info
			{
				.physicalDevice = core.gpu,
				.device = core.device,
				.instance = core.instance,
				.vulkanApiVersion = VK_API_VERSION_1_3
			};

			VK_CHECK(vmaCreateAllocator(&vma_info, &local_graphics_allocator->allocator));

			local_graphics_allocator->device = core.device;

			// init instance buffer

			local_graphics_allocator->instance_buffer = create_buffer(
				local_graphics_allocator->allocator,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_AUTO,
				VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
				sizeof(instance_t) * MAX_INSTANCES);

			// init ready-to-use meshes

			create_quad_mesh();

			create_cube_mesh();

			return true;
		}

		return false;
	}

	void destroy_graphics_allocator()
	{
		vmaDestroyAllocator(local_graphics_allocator->allocator);
	}

	pipeline_t create_graphics_pipeline(const pipeline_info_t& info)
	{
		VkDevice device = local_graphics_allocator->device;

		VkShaderModule vert = create_shader_module(device, info.v_shader);
		VkShaderModule frag = create_shader_module(device, info.f_shader);

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
			.stride = sizeof(vertex_t),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};

		VkVertexInputBindingDescription vertex_bindings[]
		{
			// Binding point 0: Mesh vertex layout description at per-vertex rate
			{
				.binding = 0,
				.stride = sizeof(vertex_t),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			},

			// Binding point 1: Instanced data at per-instance rate
			{
				.binding = 1,
				.stride = sizeof(instance_t),
				.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
			}
		};

		VkVertexInputAttributeDescription vertex_attributes[]
		{
			// Per-vertex attributes
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_t, position)},
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_t, normal)},
			{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex_t, uv)},

			// Per-instance attributes
			{3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
			{4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 4},
			{5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 8},
			{6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 12},
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
			.pColorAttachmentFormats = &local_graphics_allocator->format
		};

		VkPipelineLayoutCreateInfo layout_info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = info.set_layout_count,
			.pSetLayouts = info.set_layouts
		};

		pipeline_t current_pipeline = local_graphics_allocator->pipeline_count++;
		VkPipeline pipeline = local_graphics_allocator->pipelines[current_pipeline];
		VkPipelineLayout layout = local_graphics_allocator->layouts[current_pipeline];

		VK_CHECK(vkCreatePipelineLayout(device, &layout_info, nullptr, &layout));

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
			.layout = layout,
			.renderPass = VK_NULL_HANDLE,
			.subpass = 0
		};

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline);

		vkDestroyShaderModule(device, vert, nullptr);
		vkDestroyShaderModule(device, frag, nullptr);
		
		return current_pipeline;
	}

	mesh_atlas_t& request_graphics_allocator(size_t vertices_size, size_t indices_size)
	{
		for (uint32_t i = 0; i < MESH_ATLAS_COUNT; ++i)
		{
			if (vertices_size + local_graphics_allocator->mesh_atlas_info[i].v_used_space < MESH_ATLAS_V_SIZE  &&
				indices_size  + local_graphics_allocator->mesh_atlas_info[i].i_used_space < MESH_ATLAS_I_SIZE)
			{
				if (!local_graphics_allocator->mesh_atlas_info[i].initialized)
				{
					local_graphics_allocator->mesh_atlas_info[i].initialized = true;

					mesh_atlas_t& atlas = local_graphics_allocator->mesh_atlas[i];

					atlas.v_buffer = create_buffer(
						local_graphics_allocator->allocator,
						VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
						VMA_MEMORY_USAGE_AUTO,
						VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						MESH_ATLAS_V_SIZE);

					atlas.i_buffer = create_buffer(
						local_graphics_allocator->allocator,
						VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
						VMA_MEMORY_USAGE_AUTO,
						VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						MESH_ATLAS_I_SIZE);

				}

				local_graphics_allocator->mesh_atlas_info[i].v_used_space = vertices_size;
				local_graphics_allocator->mesh_atlas_info[i].i_used_space = indices_size;

				return local_graphics_allocator->mesh_atlas[i];
			}
		}

		LOG_ERROR(TAG_OLIVIA, "failed to allocate graphics memory");
		abort();
	}

	buffer_t create_buffer(VmaAllocator allocator, VkBufferUsageFlags usage_flags, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkDeviceSize size)
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

		buffer_t buffer{};
		VK_CHECK(vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &buffer.buffer, &buffer.allocation, &buffer.info));

		LOG_INFO(TAG_OLIVIA, "graphics-allocator: allocated buffer of [%zu] bytes", size);

		return buffer;
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


	void create_quad_mesh()
	{
		vertex_t* vertices = (vertex_t*)calloc(4, sizeof(vertex_t));
		uint32_t* indices  = (uint32_t*)calloc(6, sizeof(uint32_t));

		if (vertices && indices)
		{
			vertices[0] = { {-1.0f,-1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 0.0f, 0.0f } };
			vertices[1] = { { 1.0f,-1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 1.0f, 0.0f } };
			vertices[2] = { { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 1.0f, 1.0f } };
			vertices[3] = { {-1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },  { 0.0f, 1.0f } };

			indices[0] = 0;
			indices[1] = 1;
			indices[2] = 2;
			indices[3] = 2;
			indices[4] = 3;
			indices[5] = 0;

			size_t vertices_size = 4 * sizeof(vertex_t);
			size_t indices_size  = 6 * sizeof(uint32_t);

			mesh_atlas_t& mesh_atlas = request_graphics_allocator(vertices_size, indices_size);

			if (mesh_atlas.v_buffer.info.pMappedData && mesh_atlas.i_buffer.info.pMappedData)
			{
				memcpy(mesh_atlas.v_buffer.info.pMappedData, vertices, vertices_size);
				memcpy(mesh_atlas.i_buffer.info.pMappedData, indices, indices_size);

				mesh_t current_mesh = local_graphics_allocator->mesh_count;

				if (!mesh_atlas.initialized)
				{
					mesh_atlas.initialized = true;

					mesh_atlas.v_offset[current_mesh] = 0;
					mesh_atlas.i_offset[current_mesh] = 4 * sizeof(vertex_t);
				}
				else
				{
					mesh_t prev_mesh = local_graphics_allocator->mesh_count - 1;

					uint32_t v_offset = mesh_atlas.i_offset[prev_mesh] + mesh_atlas.i_count[prev_mesh] * sizeof(uint32_t);

					mesh_atlas.v_offset[current_mesh] = v_offset;
					mesh_atlas.i_offset[current_mesh] = v_offset + 4 * sizeof(vertex_t);
				}

				mesh_atlas.i_count[current_mesh]  = 6;

				local_graphics_allocator->quad_mesh = local_graphics_allocator->mesh_count++;
			}

			free(indices);
			free(vertices);
		}
	}

	void create_cube_mesh()
	{

	}

} // olivia