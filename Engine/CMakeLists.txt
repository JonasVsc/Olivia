include_directories(Include)
include_directories(Source)

set(OLIVIA_SOURCE
	"Source/Olivia_Graphics.cpp"
	"Source/Olivia_Runtime.cpp"
	"Source/Olivia_Events.cpp"
	"Source/Olivia_Engine.cpp"
	"Source/Olivia.cpp"
)

add_executable(Olivia ${OLIVIA_SOURCE})

target_link_libraries(Olivia PRIVATE Vulkan::Vulkan SDL3::SDL3 EnTT::EnTT)

target_include_directories(Olivia PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_include_directories(Olivia PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(Olivia PUBLIC OLIVIA_DEBUG)
endif()

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")

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

add_custom_command(TARGET Olivia POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying shaders to: $<TARGET_FILE_DIR:Olivia>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SHADER_DIR}"
        $<TARGET_FILE_DIR:Olivia>)