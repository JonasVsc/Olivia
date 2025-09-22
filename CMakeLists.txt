cmake_minimum_required(VERSION 3.31)

project("Olivia"
        VERSION 3.31
        DESCRIPTION "Olivia Gaming Platform/Engine"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

include_directories(Source)

# PLATFORM AGNOSTIC
set(OLIVIA_EDITOR "Source/Programs/Editor/Olivia.cpp")


if(WIN32)

    set(OLIVIA_PLATFORM "Source/Platform/Win32/Win32_Platform.cpp")

    # Editor
    add_executable(Olivia WIN32 ${OLIVIA_PLATFORM} ${OLIVIA_EDITOR})
    target_compile_definitions(Olivia PRIVATE OLIVIA_WIN32)

else()

    message(FATAL_ERROR "Win32 Support Only")

endif()