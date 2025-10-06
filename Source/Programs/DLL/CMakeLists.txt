if(WIN32)
    set(OLIVIA_PLATFORM 
        "${CMAKE_SOURCE_DIR}/Source/Platform/Win32/Platform.cpp")

    add_library(Game SHARED "Game.cpp")
    target_compile_definitions(Game PRIVATE OLIVIA_WIN32)
else()
    message(FATAL_ERROR "Win32 Support Only")
endif()