#include <yxis.h>
#include <Yxis/Events/IEvent.h>
#include <Yxis/Events/IKeyboardEvent.h>
#include <Yxis/Events/EventDispatcher.h>
#include <iostream>

class SandboxApplication : public Yxis::Application
{
public:
   SandboxApplication() : Application("SandboxApplication")
   {
      // do some code on initialization
      Yxis::Events::EventDispatcher::subscribe<Yxis::Events::IKeyboardEvent>(std::bind(&SandboxApplication::KeyboardHandler, this, std::placeholders::_1));
   }

   void KeyboardHandler(const std::shared_ptr<Yxis::Events::IEvent>& e)
   {
      Yxis::Events::IKeyboardEvent* kbEv = static_cast<Yxis::Events::IKeyboardEvent*>(e.get());
      
      YX_CLIENT_LOGGER->info("{} has been pressed.", kbEv->key);
      if (kbEv->key == 27)
         exit();
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