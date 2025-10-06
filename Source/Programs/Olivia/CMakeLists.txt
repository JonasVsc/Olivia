if(USE_VULKAN)
    find_package(Vulkan)
    set(OLIVIA_GRAPHICS 
        "${CMAKE_SOURCE_DIR}/Source/Graphics/Vulkan/Vulkan_Renderer.cpp")
endif()

if(WIN32)

    set(OLIVIA_PLATFORM 
        "${CMAKE_SOURCE_DIR}/Source/Platform/Win32/Platform.cpp")

    add_executable(Olivia WIN32 "Olivia.cpp" ${OLIVIA_PLATFORM} ${OLIVIA_GRAPHICS})
    target_compile_definitions(Olivia PRIVATE OLIVIA_WIN32)
else()
    message(FATAL_ERROR "Win32 Support Only")
endif()

if(USE_VULKAN)
    target_compile_definitions(Olivia PRIVATE OLIVIA_VULKAN)
    target_link_libraries(Olivia PRIVATE Vulkan::Vulkan)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug build")
    target_compile_definitions(Olivia PRIVATE OLIVIA_DEBUG)
endif()
