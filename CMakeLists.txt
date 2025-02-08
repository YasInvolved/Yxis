cmake_minimum_required(VERSION 3.21)

project(Yxis)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(3rdparty/Vulkan-Headers)
if(WIN32)
   set(VOLK_STATIC_DEFINES "VK_USE_PLATFORM_WIN32_KHR")
endif()
add_subdirectory(3rdparty/volk)
target_link_libraries(volk PUBLIC Vulkan-Headers)

# we dont need those
set(SDL_RENDER OFF)
set(SDL_RENDER_D3D OFF)
set(SDL_RENDER_D3D11 OFF)
set(SDL_RENDER_D3D12 OFF)
set(SDL_RENDER_GPU OFF)
set(SDL_RENDER_METAL OFF)
set(SDL_RENDER_VULKAN OFF)
add_subdirectory(3rdparty/SDL)

add_subdirectory(3rdparty/spdlog)

add_subdirectory(YxisEngine)
add_subdirectory(YxisSandbox)