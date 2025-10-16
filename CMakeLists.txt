cmake_minimum_required(VERSION 3.31)

project("Olivia"
        VERSION 3.31
        DESCRIPTION "Olivia Gaming Platform/Engine"
        LANGUAGES CXX)

# Shared output folder
set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})   # for executables and DLLs
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR})   # for shared libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_DIR})   # for static libraries

option(USE_VULKAN "Use Vulkan as graphics api" ON)
option(USE_SDL "Use SDL as graphics api" OFF)

set(CMAKE_CXX_STANDARD 20)

set(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source")
set(ENGINE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Engine/Include")

if(USE_VULKAN)
	find_package(Vulkan)
endif()

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.24
)

FetchContent_MakeAvailable(SDL3)


add_subdirectory("Engine")

#enable_testing()
#add_subdirectory(Tests)