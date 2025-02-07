add_library(YxisEngine SHARED "src/yxis.cpp" "include/yxis.h")

if (WIN32)
   target_compile_definitions(YxisEngine PRIVATE YX_WINDOWS YX_EXPORT_SYMBOLS)
endif()

target_compile_definitions(YxisEngine PRIVATE $<$<CONFIG:Debug>:YX_DEBUG>)
target_include_directories(YxisEngine PUBLIC "include")