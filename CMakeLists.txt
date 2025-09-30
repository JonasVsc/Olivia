cmake_minimum_required(VERSION 3.31)

project("Olivia"
        VERSION 3.31
        DESCRIPTION "Olivia Gaming Platform/Engine"
        LANGUAGES CXX)


option(USE_VULKAN "Use Vulkan as graphics api" ON)

set(CMAKE_CXX_STANDARD 20)
include_directories(Source)

# PROGRAMS
add_subdirectory(Source/Programs)

# TESTS
add_subdirectory(Thirdparty/Catch2)

enable_testing()
add_subdirectory(Tests)