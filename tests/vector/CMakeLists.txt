add_executable(test_vector "test_vector.cpp")

target_link_libraries(test_vector PRIVATE Catch2::Catch2WithMain)
target_include_directories(test_vector PRIVATE "${CMAKE_SOURCE_DIR}/engine/include")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug build")
    target_compile_definitions(test_vector PRIVATE OLIVIA_DEBUG)
endif()

