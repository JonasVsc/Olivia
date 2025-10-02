
set(OLIVIA_COMMON "${CMAKE_SOURCE_DIR}/Source/Common/Allocator.cpp")

if(WIN32)
    set(OLIVIA_PLATFORM 
        "${CMAKE_SOURCE_DIR}/Source/Platform/Win32/Win32_Platform.cpp" 
        "${CMAKE_SOURCE_DIR}/Source/Platform/Module.cpp")
    
    add_executable(TestAllocator "Test.cpp" ${OLIVIA_PLATFORM} ${OLIVIA_COMMON})
    target_compile_definitions(TestAllocator PRIVATE OLIVIA_WIN32)
endif()

target_link_libraries(TestAllocator PRIVATE Catch2::Catch2WithMain)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug build")
    target_compile_definitions(TestAllocator PRIVATE OLIVIA_DEBUG)
endif()