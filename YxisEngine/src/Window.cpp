#include "Window.h"
#include <Yxis/Logger.h>
#include <vector>
#include <cassert>
#include <algorithm>

namespace Yxis
{
   bool Window::s_windowInitialized = false;
   Window::window_t Window::s_windowHandle = nullptr;

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
   }
}