#pragma once

// all STL things and libs go here

#define VK_NO_PROTOTYPES
#define VMA_VULKAN_VERSION 1004000

#include "Vulkan/vk_enum_string_helper.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <string>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vk_mem_alloc.h>