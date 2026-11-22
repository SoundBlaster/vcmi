set(mapbuilder_SRCS
		StdInc.cpp
		MapBuilder.cpp
)

set(mapbuilder_HEADERS
		StdInc.h
)

assign_source_group(${mapbuilder_SRCS} ${mapbuilder_HEADERS})
add_executable(vcmimapbuilder ${mapbuilder_SRCS} ${mapbuilder_HEADERS})
if(USING_CONAN)
	vcmi_copy_conan_runtime_deps(vcmimapbuilder)
endif()

target_link_libraries(vcmimapbuilder PRIVATE vcmi)
target_include_directories(vcmimapbuilder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

vcmi_set_output_dir(vcmimapbuilder "")
enable_pch(vcmimapbuilder)

install(TARGETS vcmimapbuilder DESTINATION ${BIN_DIR})
