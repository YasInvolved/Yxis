add_library(YxisEngine SHARED "src/Application.cpp" "include/yxis.h" "include/Application.h" "include/definitions.h" "include/EntryPoint.h")

if (WIN32)
   target_compile_definitions(YxisEngine PRIVATE YX_WINDOWS YX_EXPORT_SYMBOLS)
endif()

target_compile_definitions(YxisEngine PRIVATE $<$<CONFIG:Debug>:YX_DEBUG>)
target_include_directories(YxisEngine PUBLIC "include")