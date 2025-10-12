include_directories(Include)
include_directories(Source)

set(OLIVIA_CORE
	"Source/Core/Olivia_Logger.cpp")

set(OLIVIA_ECS
	"Source/ECS/Olivia_ECS.cpp"
	"Source/ECS/Systems/Olivia_CameraSystem.cpp"
	"Source/ECS/Systems/Olivia_RenderSystem.cpp")

if(USE_VULKAN)
	find_package(Vulkan)
	set(OLIVIA_RENDERER
		"Source/Renderer/Vulkan_Renderer.cpp")
endif()

if(WIN32)
	set(OLIVIA_PLATFORM
		"Source/Platform/Win32_Platform.cpp")

	add_executable(Olivia WIN32 
		"Source/Olivia_Main.cpp" 
		"Source/Olivia_MeshPrimitives.cpp" 
		"Source/Olivia_Math.cpp" 
		${OLIVIA_CORE}
		${OLIVIA_ECS}
		${OLIVIA_UTILS}
		${OLIVIA_PLATFORM} 
		${OLIVIA_RENDERER})

	target_compile_definitions(Olivia PRIVATE OLIVIA_WIN32 OLIVIA_BUILD_ENGINE)
endif()

if(USE_VULKAN)
	target_compile_definitions(Olivia PRIVATE OLIVIA_VULKAN)
    target_link_libraries(Olivia PRIVATE Vulkan::Vulkan)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(Olivia PRIVATE OLIVIA_DEBUG)
endif()


add_custom_command(TARGET Olivia POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying shaders to: $<TARGET_FILE_DIR:Olivia>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Engine/Source/Shaders"
        $<TARGET_FILE_DIR:Olivia>)