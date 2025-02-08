#pragma once

#include <volk.h>
#include <string>

namespace Yxis::Vulkan 
{
   struct AppVersion
   {
      uint32_t major;
      uint32_t minor;
      uint32_t patch;
   };

   class Instance
   {
   public:
      Instance(const std::string_view applicationName, const AppVersion version);
      ~Instance();
   private:
      const std::string_view m_applicationName;
      const AppVersion m_applicationVersion;
      VkInstance m_handle = VK_NULL_HANDLE;
   };
}