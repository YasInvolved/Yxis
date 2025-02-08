#include "VulkanBackend.h"
#include <stdexcept>
#include <spdlog/fmt/fmt.h>
#include "vk_enum_string_helper.h"

namespace Yxis::Vulkan
{
   Instance::Instance(const std::string_view applicationName, const AppVersion version)
      : m_applicationName(applicationName), m_applicationVersion(version)
   {
      const VkApplicationInfo appInfo =
      {
         .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
         .pNext = nullptr,
         .pApplicationName = m_applicationName.data(),
         .applicationVersion = VK_MAKE_VERSION(m_applicationVersion.major, m_applicationVersion.minor, m_applicationVersion.patch),
         .pEngineName = "Yxis",
         .engineVersion = VK_MAKE_VERSION(1, 0, 0),
         .apiVersion = VK_API_VERSION_1_4,
      };

      const VkInstanceCreateInfo createInfo =
      {
         .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
         .pNext = nullptr,
         .flags = 0,
         .enabledLayerCount = 0,
         .ppEnabledLayerNames = nullptr,
         .enabledExtensionCount = 0,
         .ppEnabledExtensionNames = nullptr
      };

      VkResult res = vkCreateInstance(&createInfo, nullptr, &m_handle);
      if (res != VK_SUCCESS)
      {
         throw std::runtime_error(fmt::format("Failed to create Vulkan instance. {}", string_VkResult(res)));
      }

      volkLoadInstance(m_handle);
   }

   Instance::~Instance()
   {
      vkDestroyInstance(m_handle, nullptr);
   }
}