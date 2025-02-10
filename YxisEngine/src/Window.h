#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk.h>
#include <memory>
#include <string_view>
#include <vector>

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
      static VkSurfaceKHR getSurface();
      static VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkPhysicalDevice physicalDevice);
      static const surfaceformats_t getAvailableSurfaceFormats(VkPhysicalDevice physicalDevice);
      static const presentmodes_t getAvailableSurfacePresentModes(VkPhysicalDevice physicalDevice);
      static void createSurface(VkInstance instance);
      static void initialize(const std::string_view title);
   private:
      static VkInstance s_instance;
      static bool s_windowInitialized;
      static window_t s_windowHandle;
      static VkSurfaceKHR s_surfaceHandle;
   };
}