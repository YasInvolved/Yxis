#include <Yxis/Application.h>
#include <Yxis/Logger.h>
#include <SDL3/SDL.h>
#include <volk.h>
#include <vector>
#include <algorithm>
#include "VulkanBackend.h"
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

      // initialize vulkan here because it may throw an error
      Vulkan::Instance instance(m_name, { 1, 0, 0 });
      instance.initialize();
      Window::createSurface(instance.getHandle());

      Vulkan::Device device = instance.getBestDevice();
      device.initialize();

      Vulkan::Swapchain swapchain(device);
      swapchain.create();

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