function(YX_COMPILE_SHADER)
	set(options OPTIONAL)
	set(oneValue TARGET_NAME STAGE SOURCE)

	cmake_parse_arguments(PARSE_ARGV 0 arg "" "${oneValue}" "")

	get_filename_component(SRC_FILENAME "${arg_SOURCE}" NAME_WE)
	get_filename_component(SRC_PATH "${arg_SOURCE}" PATH)
	set(OUT_SHADER "${CMAKE_BINARY_DIR}/$<CONFIG>/${SRC_FILENAME}.spv")

	add_custom_target(
		${arg_TARGET_NAME}
		COMMAND glslc -fshader-stage=${arg_STAGE} "${CMAKE_CURRENT_SOURCE_DIR}/${arg_SOURCE}" -o "${OUT_SHADER}"
		COMMENT "Compiling ${arg_STAGE} shader: ${OUT_SHADER}"
	)
endfunction()