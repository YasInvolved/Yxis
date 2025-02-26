#include "Window.h"
#include <Yxis/Logger.h>
#include <vector>
#include <cassert>
#include <algorithm>

namespace Yxis
{
   bool Window::s_windowInitialized = false;
   Window::WindowPtr Window::s_windowHandle = nullptr;
   VkSurfaceKHR Window::s_surface = VK_NULL_HANDLE;

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

      SDL_Window* ptr = SDL_CreateWindow(title.data(), displayModes[0]->w, displayModes[0]->h, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
      s_windowHandle.reset(ptr);
      s_windowInitialized = true;
   }

   const std::vector<const char*> Window::getRequiredInstanceExtensions()
   {
      uint32_t extensionsCount;
      SDL_Vulkan_GetInstanceExtensions(&extensionsCount);
      std::vector<const char*> requiredExtensions(extensionsCount);
      std::memcpy(requiredExtensions.data(), SDL_Vulkan_GetInstanceExtensions(&extensionsCount), extensionsCount * sizeof(const char*));
      return requiredExtensions;
   }

   const VkSurfaceKHR Window::createSurface(const VkInstance instance)
   {
      if (not SDL_Vulkan_CreateSurface(s_windowHandle.get(), instance, nullptr, &s_surface))
         throw std::runtime_error("Failed to create surface");

      return s_surface;
   }

   const VkSurfaceKHR Window::getSurface()
   {
      assert(s_surface != VK_NULL_HANDLE && "Surface wasn't initialized");
      return s_surface;
   }

   void Window::destroySurface(const VkInstance instance)
   {
      assert(s_surface != VK_NULL_HANDLE && "Surface wasn't initialized");
      SDL_Vulkan_DestroySurface(instance, s_surface, nullptr);
   }

   Window::~Window()
   {
   }
}