#pragma once

#include "definitions.h"

namespace Yxis
{
   class YX_API Application
   {
   public:
      Application();
      virtual ~Application();

      virtual void run();
   };
}

Yxis::Application* CreateApplication();