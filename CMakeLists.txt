cmake_minimum_required(VERSION 3.31)

project("Olivia"
        VERSION 3.31
        DESCRIPTION "Olivia Engine"
        LANGUAGES CXX)

set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_DIR})

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.20
)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.11.0
)

FetchContent_MakeAvailable(SDL3)
FetchContent_MakeAvailable(Catch2)

enable_testing()

add_subdirectory("engine")
add_subdirectory("tests")
add_subdirectory("samples")