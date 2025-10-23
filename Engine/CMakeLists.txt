set(OLIVIA_SOURCE
	"src/olivia_core.cpp"
	"src/olivia_platform.cpp"
	"src/olivia_renderer.cpp"
	"src/olivia_graphics.cpp"
	"src/olivia_runtime.cpp"
	"src/olivia_context.cpp"
	"src/olivia_launcher.cpp"
)

add_executable(olivia ${OLIVIA_SOURCE})

target_link_libraries(olivia PRIVATE Vulkan::Vulkan SDL3::SDL3 EnTT::EnTT)

target_include_directories(olivia PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(olivia PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(olivia PUBLIC OLIVIA_DEBUG)
endif()

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

set(SHADERS
	"${SHADER_DIR}/olivia.vert"
	"${SHADER_DIR}/olivia.frag")

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