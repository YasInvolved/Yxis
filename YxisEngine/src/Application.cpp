#include <Yxis/Application.h>
#include <Yxis/Logger.h>
#include <Yxis/Events/EventDispatcher.h>
#include <Yxis/Events/IKeyboardEvent.h>
#include <Yxis/Events/IWindowResizedEvent.h>
#include "Vulkan/VulkanRenderer.h"
#include "Window.h"

namespace Yxis
{
   Application::Application(const std::string_view name) noexcept
      : m_name(name)
   {
      volkInitialize();
      Window::initialize(m_name);
   }

   Application::~Application()
   {
      SDL_Quit();
   }

   void Application::run()
   {
      // prevent double invoke
      if (m_running) return;
      else m_running = true;

      Vulkan::VulkanRenderer::initialize(m_name);

      while (m_running)
      {
         SDL_Event event;
         while (SDL_PollEvent(&event))
         {
             if (event.type == SDL_EVENT_QUIT) m_running = false;
             if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == false)
                 Events::EventDispatcher::dispatch(std::make_shared<Events::IKeyboardEvent>(true, event.key.key, event.key.mod));
             if (event.type == SDL_EVENT_KEY_UP && event.key.repeat == false)
                 Events::EventDispatcher::dispatch(std::make_shared<Events::IKeyboardEvent>(false, event.key.key, event.key.mod));
             if (event.type == SDL_EVENT_WINDOW_RESIZED)
                 Events::EventDispatcher::dispatch(std::make_shared<Events::IWindowResizedEvent>(event.window.data1, event.window.data2));
         }
      }

      Vulkan::VulkanRenderer::destroy();
   }

   void Application::exit()
   {
      m_running = false;
   }
}