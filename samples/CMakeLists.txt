
set(OLIVIA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/engine/include")

file(GLOB SAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(SAMPLE ${SAMPLES})

	get_filename_component(SAMPLE_NAME ${SAMPLE} NAME_WE)

	add_library(${SAMPLE_NAME} SHARED ${SAMPLE})

	target_link_libraries(${SAMPLE_NAME} PRIVATE SDL3::SDL3 Vulkan::Vulkan)
	target_include_directories(${SAMPLE_NAME} PRIVATE ${OLIVIA_INCLUDE_DIR})

endforeach()
