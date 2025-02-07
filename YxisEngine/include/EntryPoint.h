#pragma once
 
extern Yxis::Application* CreateApplication();

int main(int argc, char** argv)
{
   Yxis::Application* app = CreateApplication();
   app->run();
   delete app;

   return 0;
}