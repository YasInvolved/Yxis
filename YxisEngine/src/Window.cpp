#include "Window.h"
#include <Yxis/Logger.h>
#include "vk_enum_string_helper.h"
#include <vector>
#include <cassert>
#include <algorithm>

namespace Yxis
{
   bool Window::s_windowInitialized = false;
   Window::window_t Window::s_windowHandle = nullptr;
   VkSurfaceKHR Window::s_surfaceHandle = nullptr;
   VkInstance Window::s_instance = VK_NULL_HANDLE;

   Window::window_t& Window::getWindow()
   {
      assert(s_windowHandle != nullptr && "windowHandle is null");
      return s_windowHandle;
   }

   void Window::createSurface(VkInstance instance)
   {
      if (not SDL_Vulkan_CreateSurface(s_windowHandle.get(), instance, nullptr, &s_surfaceHandle))
      {
         throw std::runtime_error("Failed to create a Vulkan surface");
      }
   }

   VkSurfaceCapabilitiesKHR Window::getSurfaceCapabilities(VkPhysicalDevice physicalDevice)
   {
      assert(physicalDevice != VK_NULL_HANDLE && "physicalDevice is null");
      assert(s_surfaceHandle != VK_NULL_HANDLE && "s_surfaceHandle is null");
      VkSurfaceCapabilitiesKHR surfaceCapabilities;
      VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, s_surfaceHandle, &surfaceCapabilities);
      if (res != VK_SUCCESS)
      {
         throw std::runtime_error(fmt::format("Failed to fetch surface capabilities. {}", string_VkResult(res)));
      }

      return surfaceCapabilities;
   }

   const Window::surfaceformats_t Window::getAvailableSurfaceFormats(VkPhysicalDevice physicalDevice)
   {
      assert(physicalDevice != VK_NULL_HANDLE && "physicalDevice is null");
      assert(s_surfaceHandle != VK_NULL_HANDLE && "s_surfaceHandle is null");
      uint32_t formatCount;
      vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, s_surfaceHandle, &formatCount, nullptr);
      surfaceformats_t surfaceFormats(formatCount);
      VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, s_surfaceHandle, &formatCount, surfaceFormats.data());
      if (res != VK_SUCCESS)
      {
         throw std::runtime_error(fmt::format("Failed to fetch available surface formats. {}", string_VkResult(res)));
      }

      return surfaceFormats;
   }

   const Window::presentmodes_t Window::getAvailableSurfacePresentModes(VkPhysicalDevice physicalDevice)
   {
      assert(physicalDevice != VK_NULL_HANDLE && "physicalDevice is null");
      assert(s_surfaceHandle != VK_NULL_HANDLE && "s_surfaceHandle is null");
      uint32_t presentModeCount;
      vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, s_surfaceHandle, &presentModeCount, nullptr);
      presentmodes_t presentModes(presentModeCount);
      VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, s_surfaceHandle, &presentModeCount, presentModes.data());
      if (res != VK_SUCCESS)
      {
         throw std::runtime_error(fmt::format("Failed to fetch available presentation modes. {}", string_VkResult(res)));
      }

      return presentModes;
   }

   VkSurfaceKHR Window::getSurface()
   {
      assert(s_surfaceHandle != nullptr && "surfaceHandle is null");
      return s_surfaceHandle;
   }

   void Window::initialize(const std::string_view title)
   {
      if (s_windowInitialized) return;

      SDL_Init(SDL_INIT_VIDEO);
      int32_t displaysCount = 0;
      const SDL_DisplayID* displays = SDL_GetDisplays(&displaysCount);
      std::vector<const SDL_DisplayMode*> displayModes;

      for (int32_t i = 0; i < displaysCount; i++)
      {
         const auto* dm = SDL_GetCurrentDisplayMode(displays[i]);
         YX_CORE_LOGGER->info("Display found: {}", SDL_GetDisplayName(displays[i]), dm->w, dm->h, dm->refresh_rate);
         YX_CORE_LOGGER->info("Resolution: {}x{}", dm->w, dm->h);
         YX_CORE_LOGGER->info("Refresh Rate: {}hz", dm->refresh_rate);
         displayModes.emplace_back(std::move(dm));
      }

      std::sort(displayModes.begin(), displayModes.end(), [](const SDL_DisplayMode* a, const SDL_DisplayMode* b) {
         return a->w < b->w && a->refresh_rate < b->refresh_rate;
         });

      SDL_Window* ptr = SDL_CreateWindow(title.data(), displayModes[0]->w, displayModes[0]->h, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN);
      s_windowHandle.reset(ptr);
      s_windowInitialized = true;
   }

   Window::~Window()
   {
      if (s_surfaceHandle != VK_NULL_HANDLE && s_instance != VK_NULL_HANDLE)
         vkDestroySurfaceKHR(s_instance, s_surfaceHandle, nullptr);
   }
}