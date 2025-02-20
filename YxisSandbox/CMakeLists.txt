add_executable(YxisSandbox "src/main.cpp")

if (MSVC)
	target_compile_definitions(YxisSandbox PRIVATE YX_WINDOWS)
endif()


target_link_libraries(YxisSandbox PRIVATE YxisEngine)