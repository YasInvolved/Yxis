#pragma once

#include "definitions.h"
#include <string>

namespace Yxis
{
   class YX_API Application
   {
   public:
      Application(const std::string_view name) noexcept;
      virtual ~Application();

      void run();
   private:
      const std::string m_name;
      bool m_running = false;
   };
}

Yxis::Application* CreateApplication();