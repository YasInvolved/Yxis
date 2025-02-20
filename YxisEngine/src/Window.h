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

      using window_t = std::unique_ptr<SDL_Window, WindowDestructor>;
      using surfaceformats_t = std::vector<VkSurfaceFormatKHR>;
      using presentmodes_t = std::vector<VkPresentModeKHR>;

      ~Window();

      static window_t& getWindow();
      static void initialize(const std::string_view title);
   private:
      static bool s_windowInitialized;
      static window_t s_windowHandle;
   };
}