#include <Application.h>
#include <SDL3/SDL.h>

static SDL_Window* s_windowHandle = nullptr;

namespace Yxis
{
   Application::Application()
   {
      SDL_Init(SDL_INIT_VIDEO);
      s_windowHandle = SDL_CreateWindow("Application", 1280, 720, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN);
   }

   Application::~Application()
   {
      SDL_DestroyWindow(s_windowHandle);
      s_windowHandle = nullptr;
      SDL_Quit();
   }

   void Application::run()
   {
      // prevent double invoke
      if (m_running) return;
      else m_running = true;

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