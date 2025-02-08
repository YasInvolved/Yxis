#pragma once

#include "definitions.h"

namespace Yxis
{
   class YX_API Application
   {
   public:
      Application() noexcept;
      virtual ~Application();

      void run();
   private:
      bool m_running = false;
   };
}

Yxis::Application* CreateApplication();