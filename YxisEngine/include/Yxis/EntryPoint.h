#pragma once

#include <Yxis/Logger.h>

extern Yxis::Application* CreateApplication();

int main(int argc, char** argv)
{
   Yxis::Logger::initialize();

   try {
      Yxis::Application* app = CreateApplication();
      app->run();
      delete app;
   } 
   catch (const std::runtime_error& e)
   {
      Yxis::Logger::getCoreLogger()->critical(e.what());
      return -1;
   }

   return 0;
}