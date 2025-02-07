#include <yxis.h>
#include <iostream>

class SandboxApplication : public Yxis::Application
{
public:
   SandboxApplication()
   {

   }

   ~SandboxApplication()
   {

   }

   void run() override
   {
      std::cout << "Dziwki, Dragi, Lasery\n";
   }
};

Yxis::Application* CreateApplication()
{
   return new SandboxApplication();
}