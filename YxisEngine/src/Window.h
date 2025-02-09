#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <memory>
#include <string_view>

namespace Yxis
{
   struct WindowDestructor
   {
      void operator()(SDL_Window* ptr) const
      {
         SDL_DestroyWindow(ptr);
      }
   };

   using window_t = std::unique_ptr<SDL_Window, WindowDestructor>;
   static window_t s_windowHandle = nullptr;
   static VkSurfaceKHR s_surfaceHandle = nullptr;
   static bool s_windowInitialized = false;

   window_t& getWindow();
   VkSurfaceKHR getSurface();
   void createSurface(VkInstance instance);
   void initializeWindow(const std::string_view title);
}