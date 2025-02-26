#pragma once

#include "internal_pch.h"

namespace Yxis
{
   class Window 
   {
   public:
      struct WindowDestructor
      {
         void operator()(SDL_Window* ptr) const
         {
            SDL_DestroyWindow(ptr);
         }
      };

      using WindowPtr = std::unique_ptr<SDL_Window, WindowDestructor>;
      using SurfaceFormats = std::vector<VkSurfaceFormatKHR>;
      using PresentModes = std::vector<VkPresentModeKHR>;
      
      ~Window();

      static void initialize(const std::string_view title);
      static const std::vector<const char*> getRequiredInstanceExtensions();
      static const VkSurfaceKHR createSurface(const VkInstance instance);
      static const VkSurfaceKHR getSurface();
      static void destroySurface(const VkInstance instance);
   private:
      static VkSurfaceKHR s_surface;
      static bool s_windowInitialized;
      static WindowPtr s_windowHandle;
   };
}