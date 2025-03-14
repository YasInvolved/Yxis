#pragma once

#include "definitions.h"
#include "pch.h"

namespace Yxis
{
   class YX_API Application
   {
   public:
      Application(const std::string_view name) noexcept;
      virtual ~Application();

      void run();

   protected:
      void exit();
   private:
      const std::string m_name;
      bool m_running = false;
   };
}

Yxis::Application* CreateApplication();