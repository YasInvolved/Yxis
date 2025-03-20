add_executable(YxisSandbox "src/main.cpp")

if (MSVC)
	target_compile_definitions(YxisSandbox PRIVATE YX_WINDOWS)
endif()
target_link_libraries(YxisSandbox PRIVATE YxisEngine)

YX_COMPILE_SHADER(TARGET_NAME YxisSandbox_VERTSHADER STAGE "vert" SOURCE "shaders/vs_main.hlsl")
YX_COMPILE_SHADER(TARGET_NAME YxisSandbox_FRAGSHADER STAGE "frag" SOURCE "shaders/fs_main.hlsl")
add_dependencies(YxisSandbox YxisSandbox_VERTSHADER YxisSandbox_FRAGSHADER)