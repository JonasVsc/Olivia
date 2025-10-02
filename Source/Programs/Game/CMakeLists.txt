cmake_minimum_required(VERSION 3.31)

# PLATFORM AGNOSTIC
set(OLIVIA_COMMON "${CMAKE_SOURCE_DIR}/Source/Common/Allocator.cpp")

# GRAPHICS API
if(USE_VULKAN)
    find_package(Vulkan)
    set(OLIVIA_GRAPHICS 
        "${CMAKE_SOURCE_DIR}/Source/Renderer/Vulkan/Vulkan_Renderer.cpp"
        "${CMAKE_SOURCE_DIR}/Source/Graphics/Vulkan/Vulkan_Graphics.cpp"
        "${CMAKE_SOURCE_DIR}/Source/Graphics/Vulkan/Vulkan_Buffer.cpp")
endif()

if(WIN32)

    set(OLIVIA_PLATFORM 
        "${CMAKE_SOURCE_DIR}/Source/Platform/Win32/Win32_Platform.cpp"
        "${CMAKE_SOURCE_DIR}/Source/Platform/Module.cpp")

    add_executable(Olivia WIN32 "Olivia.cpp" ${OLIVIA_PLATFORM} ${OLIVIA_GRAPHICS} ${OLIVIA_COMMON})
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



# GAME SAMPLE
if(WIN32)

    set(OLIVIA_PLATFORM 
    "${CMAKE_SOURCE_DIR}/Source/Platform/Win32/Win32_Platform.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Platform/Module.cpp")

    add_library(OliviaGame SHARED "Olivia_Game.cpp" ${OLIVIA_PLATFORM} ${OLIVIA_GRAPHICS} ${OLIVIA_COMMON})
    target_compile_definitions(OliviaGame PRIVATE OLIVIA_WIN32)
else()
    message(FATAL_ERROR "Win32 Support Only")
endif()

if(USE_VULKAN)
    target_compile_definitions(OliviaGame PRIVATE OLIVIA_VULKAN)
    target_link_libraries(OliviaGame PRIVATE Vulkan::Vulkan)
endif()