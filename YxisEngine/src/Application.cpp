#include <Yxis/Application.h>
#include <Yxis/Logger.h>
#include <SDL3/SDL.h>
#include <volk.h>
#include <vector>
#include <algorithm>
#include "VulkanBackend.h"

struct WindowDestructor {
   void operator()(SDL_Window* ptr) const
   {
      SDL_DestroyWindow(ptr);
   }
};

static std::unique_ptr<SDL_Window, WindowDestructor> s_windowHandle;

namespace Yxis
{
   Application::Application() noexcept
   {
      volkInitialize();
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

      s_windowHandle.reset(SDL_CreateWindow("Application", displayModes[0]->w, displayModes[0]->h, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN));
   }

   Application::~Application()
   {
      SDL_DestroyWindow(s_windowHandle.get());
      s_windowHandle = nullptr;
      SDL_Quit();
   }

   void Application::run()
   {
      // prevent double invoke
      if (m_running) return;
      else m_running = true;

      // initialize vulkan here because it may throw an error
      Vulkan::Instance instance("Sigma", { 1, 0, 0 });

      while (m_running)
      {
         SDL_Event event;
         while (SDL_PollEvent(&event))
         {
            if (event.type == SDL_EVENT_QUIT) m_running = false;
         }
      }
   }
}