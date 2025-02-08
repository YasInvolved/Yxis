#pragma once

#include <Yxis/Logger.h>

extern Yxis::Application* CreateApplication();

int main(int argc, char** argv)
{
   Yxis::Logger::initialize();
   Yxis::Application* app = CreateApplication();

   try 
   {
      app->run();
   } 
   catch (const std::runtime_error& e)
   {
      YX_CORE_LOGGER->critical(e.what());
      return -1;
   }

   delete app;

   return 0;
}