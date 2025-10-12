include_directories(Include)
include_directories(Source)



if(WIN32)
    set(OLIVIA_PLATFORM 
        "${CMAKE_SOURCE_DIR}/Source/Platform/Win32/Platform.cpp")

    add_library(Game SHARED "Source/Game.cpp")
    target_include_directories(Game PRIVATE ${ENGINE_INCLUDE_DIR})
    target_compile_definitions(Game PRIVATE OLIVIA_WIN32 OLIVIA_USE_DLL)
else()
    message(FATAL_ERROR "Win32 Support Only")
endif()