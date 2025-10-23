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

find_package(Vulkan)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.20
)

FetchContent_MakeAvailable(SDL3)

add_executable(olivia "src/olivia.cpp" "src/olivia_types.h")

target_link_libraries(olivia PRIVATE SDL3::SDL3 Vulkan::Vulkan)
target_include_directories(olivia PRIVATE include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(olivia PUBLIC OLIVIA_DEBUG)
endif()


set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

set(SHADERS
	"${SHADER_DIR}/olivia2d.vert"
	"${SHADER_DIR}/olivia2d.frag"
	"${SHADER_DIR}/olivia3d.vert"
	"${SHADER_DIR}/olivia3d.frag")

foreach(SHADER ${SHADERS})
	get_filename_component(FILE_NAME ${SHADER} NAME)
	set(SPIRV ${SHADER}.spv)

	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND glslangValidator -V ${SHADER} -o ${SPIRV}
		DEPENDS ${SHADER}
		COMMENT "Compiling shader ${FILE_NAME}"
		VERBATIM)

	list(APPEND COMPILED_SHADERS ${SPIRV})
endforeach()

add_custom_target(
	compile_shaders ALL
	DEPENDS ${COMPILED_SHADERS}
)

add_custom_command(TARGET olivia POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying shaders to: $<TARGET_FILE_DIR:olivia>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SHADER_DIR}"
        $<TARGET_FILE_DIR:olivia>)