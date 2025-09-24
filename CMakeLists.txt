cmake_minimum_required(VERSION 3.31)

project("Olivia"
        VERSION 3.31
        DESCRIPTION "Olivia Gaming Platform/Engine"
        LANGUAGES CXX)


option(USE_VULKAN "Use Vulkan as graphics api" ON)

set(CMAKE_CXX_STANDARD 20)
include_directories(Source)

# PLATFORM AGNOSTIC
set(OLIVIA_EDITOR "Source/Programs/Editor/Olivia.cpp")

# GRAPHICS API
if(USE_VULKAN)
    find_package(Vulkan)
    set(OLIVIA_GRAPHICS "Source/Renderer/Vulkan/Vulkan_Renderer.cpp")
endif()

if(WIN32)

    set(OLIVIA_PLATFORM "Source/Platform/Win32/Win32_Platform.cpp")

    # Editor
    add_executable(Olivia WIN32 ${OLIVIA_PLATFORM} ${OLIVIA_EDITOR} ${OLIVIA_GRAPHICS})
    target_compile_definitions(Olivia PRIVATE OLIVIA_WIN32)
else()
    message(FATAL_ERROR "Win32 Support Only")
endif()


# LINKER

if(USE_VULKAN)
    target_compile_definitions(Olivia PRIVATE OLIVIA_VULKAN)
    target_link_libraries(Olivia PRIVATE Vulkan::Vulkan)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug build")
    target_compile_definitions(Olivia PRIVATE OLIVIA_DEBUG)
endif()