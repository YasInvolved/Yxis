#include <yxis.h>
#include <Yxis/Events/IEvent.h>
#include <Yxis/Events/IKeyboardEvent.h>
#include <Yxis/Events/EventDispatcher.h>
#include <iostream>

static void KeyboardHandler(const std::shared_ptr<Yxis::Events::IEvent>& e)
{
    auto* kb = static_cast<Yxis::Events::IKeyboardEvent*>(e.get());
    YX_CLIENT_LOGGER->info("{} is {} with mod {}", kb->key, kb->down ? "down" : "up", kb->mod);
}

class SandboxApplication : public Yxis::Application
{
public:
   SandboxApplication() : Application("SandboxApplication")
   {
      // do some code on initialization
      Yxis::Events::EventDispatcher::subscribe<Yxis::Events::IKeyboardEvent>(KeyboardHandler);
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