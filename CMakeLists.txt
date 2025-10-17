cmake_minimum_required(VERSION 3.31)

project("Olivia"
        VERSION 3.31
        DESCRIPTION "Olivia Platform/Engine"
        LANGUAGES CXX)

# Shared output folder
set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})   # for executables and DLLs
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR})   # for shared libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_DIR})   # for static libraries

set(CMAKE_CXX_STANDARD 20)

set(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source")
set(ENGINE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Engine/Include")

find_package(Vulkan)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.20
)

FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.15.0
)

FetchContent_MakeAvailable(SDL3)
FetchContent_MakeAvailable(EnTT)

add_subdirectory("Engine")