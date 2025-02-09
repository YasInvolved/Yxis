#include <yxis.h>
#include <iostream>

class SandboxApplication : public Yxis::Application
{
public:
   SandboxApplication() : Application("SandboxApplication")
   {
      // do some code on initialization
   }

   ~SandboxApplication()
   {
      // do some code on exit
   }
};

Yxis::Application* CreateApplication()
{
   return new SandboxApplication();
}