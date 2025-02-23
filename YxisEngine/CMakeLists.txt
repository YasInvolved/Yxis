add_library(YxisEngine SHARED "src/Application.cpp" "include/yxis.h" "include/Yxis/Application.h" "include/Yxis/definitions.h" "include/Yxis/EntryPoint.h" "include/Yxis/Logger.h" "src/Logger.cpp"   "src/Window.h" "src/Window.cpp" "src/Vulkan/VulkanRenderer.h" "src/Vulkan/VulkanRenderer.cpp" "src/internal_pch.h" "include/Yxis/Events/IEvent.h"     "include/Yxis/Events/IKeyboardEvent.h"   "include/Yxis/Events/EventDispatcher.h" "src/Events/EventDispatcher.cpp" "include/Yxis/pch.h"   "include/Yxis/Events/IWindowResizedEvent.h"   "src/Vulkan/VulkanUtilities.h" "src/Vulkan/VulkanUtilities.cpp" "src/Vulkan/Device.h" "src/Vulkan/Device.cpp")

if (WIN32)
   target_compile_definitions(YxisEngine PRIVATE YX_WINDOWS YX_EXPORT_SYMBOLS)
endif()

target_compile_definitions(YxisEngine PRIVATE $<$<CONFIG:Debug>:YX_DEBUG>)
target_include_directories(YxisEngine PUBLIC "include")
target_precompile_headers(YxisEngine PRIVATE "src/internal_pch.h" PUBLIC "include/Yxis/pch.h")
target_link_libraries(YxisEngine PRIVATE SDL3::SDL3 volk::volk PUBLIC spdlog::spdlog)