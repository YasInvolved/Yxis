add_library(YxisEngine SHARED "src/Application.cpp" "include/yxis.h" "include/Yxis/Application.h" "include/Yxis/definitions.h" "include/Yxis/EntryPoint.h" "include/Yxis/Logger.h" "src/Logger.cpp" "src/VulkanBackend.h" "src/VulkanBackend.cpp")

if (WIN32)
   target_compile_definitions(YxisEngine PRIVATE YX_WINDOWS YX_EXPORT_SYMBOLS)
endif()

target_compile_definitions(YxisEngine PRIVATE $<$<CONFIG:Debug>:YX_DEBUG>)
target_include_directories(YxisEngine PUBLIC "include")
target_link_libraries(YxisEngine PRIVATE SDL3::SDL3 volk::volk PUBLIC spdlog::spdlog)